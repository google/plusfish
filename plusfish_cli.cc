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

#include <time.h>

#include <csignal>
#include <memory>
#include <sstream>
#include <string>


#include <glog/logging.h>
#include "absl/flags/config.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/flags/usage_config.h"
#include "absl/strings/match.h"
#include "audit/matchers/matcher_factory.h"
#include "audit/passive_auditor.h"
#include "audit/response_time_check.h"
#include "audit/selective_auditor.h"
#include "crawler.h"
#include "curl_http_client.h"
#include "datastore.h"
#include "hidden_objects_finder.h"
#include "http_client.h"
#include "not_found_detector.h"
#include "plusfish.h"
#include "proto/http_request.pb.h"
#include "proto/security_check.pb.h"
#include "request.h"
#include "util/clock.h"
#include "util/config.h"

ABSL_FLAG(std::string, exclude_regex_list, "",
          "A comma separated list of regular expressions. URLs that match  "
          "a regex in this list will be excluded from the scan.");
ABSL_FLAG(std::string, include_regex_list, "",
          "A comma separated list of regular expressions. Only URLs that "
          "a regex in this list will be included in the scan.");
ABSL_FLAG(std::string, checks_config_path, "",
          "Specifies the config(s) that should be loaded. Use * to load "
          "multiple files at once.");
ABSL_FLAG(std::string, report_type, "",
          "A comma separated list of report types (TEXT or JSON).");
ABSL_FLAG(int32_t, max_scan_duration_sec, 86400,
          "Stop scanning after this duration and exit cleanly. Use 0 to "
          "disable.");
ABSL_FLAG(int32_t, max_request_rate_sec, 100,
          "The maximum amount of new requests to schedule per second.");
ABSL_FLAG(int32_t, max_auditor_runners, 10,
          "The maximum number of active auditor runners. For every URL, one "
          "runner is responsible for running all security test. More "
          "runners can increase performance but at the cost of memory "
          "and CPU utilisation.");
ABSL_FLAG(int32_t, response_time_threshold_ms, 8000,
          "Report requests where the average response time exceeds this "
          "value.");
ABSL_FLAG(int32_t, response_time_measurements, 3,
          "The number of times to repeat a request before calcularing the "
          " average response time.");
ABSL_FLAG(std::string, default_headers, "",
          "A default set of headers to include in all requests. "
          "Format to use: domain:header=value, ...");
ABSL_FLAG(std::string, bruteforce_wordlist, "",
          "The wordlist for finding hidden files and directories.");
ABSL_FLAG(std::string, bruteforce_extensions, "",
          "The extensions to use for finding hidden files.");
ABSL_FLAG(std::string, requests_asciipb_file, "",
          "An ascii protobuf file containing a collection of HttpRequest "
          "instances to start the scan with.");

using plusfish::DataStore;
using plusfish::Request;

// Hold's a reference to the plusfish signal handler function. This is called
// via the wrapper and used to shutdown scans.
static std::function<void(int sig)> signal_handler;

// A helper function to call the plusfish instance signal handler.
static void SignalWrapper(int sig) { signal_handler(sig); }

std::vector<std::string> SplitString(std::string value, char delimiter) {
  std::vector<std::string> result;
  std::stringstream ss(value);
  while (ss.good()) {
    std::string substr;
    getline(ss, substr, ',');
    result.push_back(substr);
  }
  return result;
}

// Returns the modules to show the help flags from when plusfish is called with
// --help or --helpshort.
bool ContainsHelpshortFlags(absl::string_view filename) {
  return absl::StrContains(filename, "curl") ||
         absl::StrContains(filename, "plusfish");
}

// Install Abseil Flags' library usage callbacks. This needs to be done
// before any operation that may call one of the callbacks.
void InstallFlagsUsageConfig() {
  absl::FlagsUsageConfig config;
  config.contains_help_flags = ContainsHelpshortFlags;
  config.contains_helpshort_flags = ContainsHelpshortFlags;
  absl::SetFlagsUsageConfig(config);
}

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage(
      absl::StrCat("usage: ", argv[0], " [options] <url> [<url>...]"));
  google::InitGoogleLogging(argv[0]);

  InstallFlagsUsageConfig();
  std::vector<char*> urls = absl::ParseCommandLine(argc, argv);
  plusfish::DataStore datastore;
  plusfish::MatcherFactory matcher_factory;
  plusfish::SecurityCheckConfig checks_config;
  std::unique_ptr<plusfish::NotFoundDetector> not_found_detector_;
  std::unique_ptr<plusfish::HiddenObjectsFinder> objects_finder_;
  plusfish::CurlHttpClient http_client(
      absl::GetFlag(FLAGS_max_request_rate_sec));
  std::unique_ptr<plusfish::PassiveAuditor> passive_auditor;
  std::unique_ptr<plusfish::SelectiveAuditor> selective_auditor;
  if (!http_client.Initialize()) {
    DLOG(ERROR) << "Unable to initialize HTTP client.";
    return 1;
  }

  auto issue_callback = std::bind(&DataStore::AddIssue, &datastore,
                                  std::placeholders::_1, std::placeholders::_2,
                                  std::placeholders::_3, std::placeholders::_4);
  auto set_meta_callback = std::bind(
      &DataStore::AddRequestMetadata, &datastore, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3);

  auto get_meta_callback = std::bind(
      &DataStore::GetRequestMetadata, &datastore, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3);

  matcher_factory.SetRequestMetaCallback(get_meta_callback);

  if (!absl::GetFlag(FLAGS_checks_config_path).empty()) {
    selective_auditor.reset(
        new plusfish::SelectiveAuditor(&matcher_factory, &http_client));
    passive_auditor.reset(new plusfish::PassiveAuditor(&matcher_factory));

    selective_auditor->SetRegisterIssueCallback(issue_callback);
    selective_auditor->SetRequestMetaCallback(set_meta_callback);
    selective_auditor->SetGetRequestMetaCallback(get_meta_callback);
    passive_auditor->SetRegisterIssueCallback(issue_callback);
    // TODO: Move the check creation to a factory.
    selective_auditor->AddSecurityCheck(new plusfish::ResponseTimeCheck(
        absl::GetFlag(FLAGS_response_time_measurements),
        absl::GetFlag(FLAGS_response_time_threshold_ms)));

    if (!plusfish::util::LoadCheckConfigs(
            absl::GetFlag(FLAGS_checks_config_path), &checks_config)) {
      LOG(ERROR) << "Unable to load security checks from: "
                 << absl::GetFlag(FLAGS_checks_config_path);
      exit(1);
    }

    for (const auto& check : checks_config.security_test()) {
      if (!check.has_generator_rule() && !check.has_matching_rule()) {
        LOG(ERROR) << "Incomplete security check: " << check.DebugString();
        exit(1);
      }

      // Left right left right left right ;-)
      if (!check.has_generator_rule()) {
        passive_auditor->AddSecurityTest(check);
      } else {
        selective_auditor->AddSecurityTest(check);
      }
    }
  }

  // Set the default (optionally domain specific) headers.
  if (!absl::GetFlag(FLAGS_default_headers).empty()) {
    std::vector<std::string> split =
        SplitString(absl::GetFlag(FLAGS_default_headers), ',');
    for (const auto& header : split) {
      // TODO: Improve header splitting.
      std::vector<std::string> parts = SplitString(header, ':');
      if (parts.size() != 3) {
        LOG(ERROR) << "Invalid header value: " << header
                   << ". Use <domain>:<name>:<value> or *:<name>:<value>";
        exit(1);
      }

      if (!http_client.RegisterDefaultHeader(parts[0], parts[1], parts[2])) {
        LOG(ERROR) << "Unable to set header: " << header;
        exit(1);
      }
    }
  }

  plusfish::Crawler crawler(&http_client, selective_auditor.get(),
                            passive_auditor.get(), &datastore);
  // Load the whitelist.
  if (!absl::GetFlag(FLAGS_include_regex_list).empty()) {
    std::vector<std::string> split_result =
        SplitString(absl::GetFlag(FLAGS_include_regex_list), ',');
    for (const auto& include : split_result) {
      datastore.AddWhitelistRegex(include);
    }
  }

  // Load the blacklist.
  if (!absl::GetFlag(FLAGS_exclude_regex_list).empty()) {
    std::vector<std::string> split_result =
        SplitString(absl::GetFlag(FLAGS_exclude_regex_list), ',');
    for (const auto& exclude : split_result) {
      datastore.AddBlacklistRegex(exclude);
    }
  }

  // Prepare the 404 detector.
  not_found_detector_.reset(new plusfish::NotFoundDetector());
  not_found_detector_->SetDatastoreFingerprintCallback(
      std::bind(&plusfish::DataStore::AddFileNotFoundHtmlFingerprint,
                &datastore, std::placeholders::_1));

  not_found_detector_->SetHttpClientScheduleCallback(std::bind(
      static_cast<bool (plusfish::CurlHttpClient::*)(plusfish::Request*)>(
          &plusfish::CurlHttpClient::Schedule),
      &http_client, std::placeholders::_1));

  if (!absl::GetFlag(FLAGS_bruteforce_wordlist).empty()) {
    objects_finder_.reset(new plusfish::HiddenObjectsFinder(
        std::bind(
            static_cast<bool (plusfish::CurlHttpClient::*)(plusfish::Request*)>(
                &plusfish::CurlHttpClient::Schedule),
            &http_client, std::placeholders::_1),
        std::bind(&plusfish::DataStore::IsFileNotFoundHtmlFingerprint,
                  &datastore, std::placeholders::_1),
        std::bind(&plusfish::DataStore::AddRequest, &datastore,
                  std::placeholders::_1),
        std::bind(&plusfish::DataStore::AddIssueById, &datastore,
                  std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3)));

    objects_finder_->LoadWordlistFromFile(
        absl::GetFlag(FLAGS_bruteforce_wordlist));
    if (!absl::GetFlag(FLAGS_bruteforce_extensions).empty()) {
      objects_finder_->LoadExtensionsFromFile(
          absl::GetFlag(FLAGS_bruteforce_extensions));
    }
  }

  plusfish::Clock clock;
  plusfish::Plusfish plusfish(
      &clock, &crawler, selective_auditor.release(),
      not_found_detector_.release(), objects_finder_.release(),
      absl::GetFlag(FLAGS_max_auditor_runners), &http_client, &datastore);

  // Signals can only be set after the plusfish instance is created.
  signal_handler = std::bind(&plusfish::Plusfish::SignalHandler, &plusfish,
                             std::placeholders::_1);
  signal(SIGINT, SignalWrapper);

  if (absl::GetFlag(FLAGS_max_scan_duration_sec) > 0) {
    plusfish.SetShutdownTime(time(nullptr) +
                             absl::GetFlag(FLAGS_max_scan_duration_sec));
  }

  if (!absl::GetFlag(FLAGS_report_type).empty()) {
    if (!plusfish.InitReporting(absl::GetFlag(FLAGS_report_type))) {
      LOG(ERROR) << "Could not initialize reporting";
      exit(1);
    }
  }

  bool loaded_files_from_pb = false;
  if (!absl::GetFlag(FLAGS_requests_asciipb_file).empty()) {
    plusfish::HttpRequestCollection collection;
    if (!plusfish::util::LoadRequestsConfig(
            absl::GetFlag(FLAGS_requests_asciipb_file), &collection)) {
      LOG(ERROR) << "Unable to load requests from: "
                 << absl::GetFlag(FLAGS_requests_asciipb_file);
      exit(1);
    }

    for (const plusfish::HttpRequest& req_proto : collection.request()) {
      std::unique_ptr<Request> req(new Request(req_proto));
      datastore.AddRequest(std::move(req));
      loaded_files_from_pb = true;
    }
  }

  // Load URLs from the command-line while skipping the first one which is the
  // binary name.
  urls.erase(urls.begin());

  if (urls.empty() && !loaded_files_from_pb) {
    LOG(WARNING) << "No URLs/requests were given to scan. Cannot proceed. ";
    exit(1);
  }

  for (const auto& url : urls) {
    if (plusfish.AddURL(url)) {
      LOG(INFO) << "Added URL: " << url;
    } else {
      LOG(ERROR) << "Unable to add URL: " << url;
      exit(1);
    }
  }

  plusfish.Run();
  plusfish.Report(checks_config);
  return 0;
}
