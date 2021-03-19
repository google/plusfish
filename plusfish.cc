// Copyright 2020 Google LLC. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "plusfish.h"

#include <time.h>

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <sstream>

#include "opensource/deps/base/integral_types.h"
#include <glog/logging.h>
#include "absl/flags/flag.h"
#include "absl/synchronization/mutex.h"
#include "audit/selective_auditor.h"
#include "crawler.h"
#include "datastore.h"
#include "http_client.h"
#include "proto/security_check.pb.h"
#include "report/reporter.h"
#include "report/reporter_factory.h"
#include "request.h"
#include "request_handler.h"
#include "util/clock.h"
#include "util/config.h"

ABSL_FLAG(int32_t, max_graceful_shutdown_duration, 300,
          "When plusfish is asked to gracefully shutdown (e.g. via SIGINT), "
          "perform an exit after this amount of seconds. This regardless of "
          "whether all existing connections have completed.");
ABSL_FLAG(bool, enable_console_reporting, true, "Enable the console reporting");

// Delay in milliseconds to introduce when our main plusfish loop is too fast.
static const int kFastLoopDelayMs = 2;
// Max time the loop can take before sending warning messages.
static const int kLoopDelayWarningThresholdMs = 1000;
// Amount of hidden object test URLs to schedule per loop iteration.
static const int kMaxHiddenObjectsUrlsToSchedule = 100;
// Terminal special characters to decorate text.
static const char* kTermBold = "\u001b[1m";
static const char* kTermColorReset = "\u001b[0m";
static const char* kTermClearScreen = "\u001b[H\x1b[2J";

namespace plusfish {

Plusfish::Plusfish(Clock* clock, Crawler* crawler, SelectiveAuditor* auditor,
                   NotFoundDetector* not_found_detector,
                   HiddenObjectsFinder* objects_finder, int max_auditor_runners,
                   HttpClientInterface* http_client, DataStore* datastore)
    : crawler_(crawler),
      clock_(clock),
      selective_auditor_(auditor),
      max_auditor_runners_(max_auditor_runners),
      http_client_(http_client),
      datastore_(datastore),
      enabled_(true),
      shutdown_time_ms_(kint64max) {
  not_found_detector_ = not_found_detector;
  objects_finder_ = objects_finder;
}

Plusfish::~Plusfish() {}

bool Plusfish::enabled() {
  absl::ReaderMutexLock l(&enabled_mutex_);
  return enabled_;
}

int64 Plusfish::shutdown_time() {
  absl::ReaderMutexLock l(&shutdown_time_mutex_);
  return shutdown_time_ms_;
}

void Plusfish::SetShutdownTime(int64 time_ms) {
  absl::MutexLock l(&shutdown_time_mutex_);
  shutdown_time_ms_ = time_ms;
}

void Plusfish::Disable() {
  absl::MutexLock l(&enabled_mutex_);
  DLOG(INFO) << "Disabling the plusfish scanner.";
  enabled_ = false;
}

bool Plusfish::KeepRunningOrShutdown() {
  if (!enabled()) {
    return false;
  }
  // If the shutdown time is still in the future: continue running.
  if (shutdown_time() > time(nullptr)) {
    return true;
  }

  DLOG(INFO) << "Shutdown timeout reached (http client enabled: "
             << http_client_->enabled() << ")";
  if (http_client_->enabled()) {
    ShutdownGraceful();
    return true;
  }
  Shutdown();
  return false;
}

void Plusfish::SignalHandler(int sig) {
  switch (sig) {
    case SIGINT:
      // The first ^C will stop the HTTP client. The second will cause plusfish
      // to exit.
      if (http_client_->enabled()) {
        ShutdownGraceful();
      } else {
        Shutdown();
      }
      break;

    case SIGTERM:
      LOG(INFO) << "Received SIGTERM signal.";
      Shutdown();
      break;

    default:
      DLOG(INFO) << "Received unhandled signal: " << sig;
  }
}

void Plusfish::ShutdownGraceful() {
  LOG(WARNING) << "Graceful shutdown requested.";
  if (http_client_->enabled()) {
    LOG(INFO) << "Shutdown called: Disabling HTTP client.";
    http_client_->Disable();
  }
  // Set the shutdown time so that the graceful exit (which includes waiting for
  // active connections to finish) cannot last until the end of days.
  SetShutdownTime(time(nullptr) +
                  absl::GetFlag(FLAGS_max_graceful_shutdown_duration));
}

void Plusfish::Shutdown() {
  LOG(WARNING) << "Immediate shutdown requested.";
  // Eventhough this will cause an almost immediate shutdown of Plusfish, we
  // still disable the HTTP client: active connections (in-air) will keep
  // calling it in the background.
  if (http_client_->enabled()) {
    LOG(INFO) << "Shutdown called: Disabling HTTP client.";
    http_client_->Disable();
  }
  Disable();
}

bool Plusfish::InitReporting(const std::string& report_types) {
  ReporterFactory factory;
  std::stringstream ss(report_types);
  while (ss.good()) {
    std::string report_type;
    std::getline(ss, report_type, ',');

    ReporterInterface* report = factory.GetReporterByName(report_type);
    if (report != nullptr) {
      reporters_.emplace_back(report);
    } else {
      LOG(WARNING) << "Could not load reporter type: " << report_type;
      return false;
    }
  }
  return true;
}

bool Plusfish::AddURL(const std::string& url) {
  std::unique_ptr<Request> new_request(new Request(url));
  if (!new_request->url_is_valid()) {
    return false;
  }

  datastore_->AddHost(new_request->host());
  std::string new_url = new_request->url();
  if (!datastore_->AddRequest(std::move(new_request))) {
    LOG(WARNING) << "Unable to add request:" << new_url;
  }
  return true;
}

bool Plusfish::Report(const SecurityCheckConfig& config) {
  if (reporters_.empty()) {
    return false;
  }
  for (const auto& reporter : reporters_) {
    reporter->ReportSecurityConfig(config);
  }
  datastore_->Report(reporters_);
  return true;
}

void Plusfish::Run() {
  int running_requests;

  do {
    size_t begin_time_ms = clock_->EpochTimeInMilliseconds();
    if (http_client_->StartNewRequests()) {
      DLOG(INFO) << "Started new requests";
    }

    running_requests = http_client_->Poll();
    if (absl::GetFlag(FLAGS_enable_console_reporting)) {
      const auto& issue_counters = datastore_->issue_count_per_severity();
      printf(
          "%s%s Plusfish scan statistics%s\n\n"
          "        Request Queue : %u running, %u pending, %u done\n"
          "          Audit Queue : %u pending, %u runners\n"
          "               Issues : %u critical, %u high, %u medium, %u low\n"
          "        Hidden object : %u found, %u URLs pending, %u req queue "
          "size\n",
          kTermClearScreen, kTermBold, kTermColorReset,
          (uint32_t)http_client_->active_requests_count(),
          (uint32_t)http_client_->schedule_queue_size(),
          (uint32_t)http_client_->requests_performed_count(),
          (uint32_t)datastore_->audit_queue_size(),
          selective_auditor_ ? (uint32_t)selective_auditor_->runner_count() : 0,
          issue_counters.find(Severity::CRITICAL)->second,
          issue_counters.find(Severity::HIGH)->second,
          issue_counters.find(Severity::MODERATE)->second,
          issue_counters.find(Severity::LOW)->second,
          (uint32_t)objects_finder_->num_objects_found(),
          (uint32_t)objects_finder_->pending_urls_count(),
          (uint32_t)objects_finder_->test_urls_queue_count());
    }

    if (datastore_->probe_queue_size()) {
      const Request* req = datastore_->GetRequestFromProbeQueue();
      if (req) {
        if (not_found_detector_->AddUrl(req->url()) && objects_finder_) {
          objects_finder_->AddUrl(req->url());
        }

        // In this case we don't wait for the previous requests to have
        // completed (some of them are allowed to fail) and directly add the
        // request in the crawl queue.
        datastore_->AddRequestToCrawlQueue(
            datastore_->GetRequestById(req->id()));
      }
    }

    if (datastore_->crawl_queue_size()) {
      const Request* req = datastore_->GetRequestFromCrawlQueue();
      if (req && !crawler_->Crawl(datastore_->GetRequestById(req->id()))) {
        DLOG(WARNING) << "Unable to crawl: " << req->url();
      }
    }

    objects_finder_->ScheduleRequests(kMaxHiddenObjectsUrlsToSchedule);

    if (selective_auditor_ && datastore_->audit_queue_size() > 0 &&
        selective_auditor_->runner_count() < max_auditor_runners_) {
      // Get a request from the queue and start the security tests.
      const Request* req = datastore_->GetRequestFromAuditQueue();
      if (!selective_auditor_->ScheduleFirst(req)) {
        DLOG(INFO) << "No security tests scheduled for: " << req->url();
      }
    }

    size_t run_time_ms = clock_->EpochTimeInMilliseconds() - begin_time_ms;
    if (run_time_ms > kLoopDelayWarningThresholdMs) {
      LOG_EVERY_N(WARNING, 60)
          << "Main loop delay exceeds threshold and is at: " << run_time_ms
          << "ms. Consider reducing the amount of concurrent connections.";
    } else if (run_time_ms < kFastLoopDelayMs) {
      DLOG(INFO) << "Delaying loop with " << run_time_ms - kFastLoopDelayMs
                 << "ms";
      clock_->SleepMilliseconds(kFastLoopDelayMs - run_time_ms);
    }
  } while (KeepRunningOrShutdown() && (http_client_->active_requests_count() ||
                                       http_client_->schedule_queue_size()));
}

}  // namespace plusfish
