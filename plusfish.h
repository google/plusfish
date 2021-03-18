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

#ifndef PLUSFISH_PLUSFISH_H_
#define PLUSFISH_PLUSFISH_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"
#include "hidden_objects_finder.h"
#include "not_found_detector.h"
#include "util/clock.h"

namespace plusfish {

class Crawler;
class DataStore;
class HttpClientInterface;
class Request;
class ReporterInterface;
class SecurityCheckConfig;
class SelectiveAuditor;

class Plusfish {
 public:
  // Create a plusfish instance. The crawler, auditor, http client,
  // datastore instance and clock are all expected to have been initialized.
  // Does not take ownership.
  Plusfish(Clock* clock, Crawler* crawler, SelectiveAuditor* auditor,
           NotFoundDetector* not_found_detector,
           HiddenObjectsFinder* objects_finder, int max_auditor_runners,
           HttpClientInterface* http_client, DataStore* datastore);
  ~Plusfish();

  // Indicates whether plusfish is enabled nor not.
  bool enabled();
  // Returns the planned shutdown time. If no time is set (yet), max int is
  // returned.
  int64 shutdown_time();
  // Indicates whether plusfish should keep running or not. The answer depends
  // on whether plusfish is this enabled; a graceful shutdown was requested and
  // how long ago that request was done.
  bool KeepRunningOrShutdown();
  // Run plusfish. This method will not return unless either all targets URLs
  // have been tested or the plusfish instance is disabled (via Disable() call).
  // If one a timeout is reached, this method will also call one of the shutdown
  // routines.
  void Run();
  // Report the results. Returns true on success and false on failure.
  bool Report(const SecurityCheckConfig& config);
  // Add the given URL to the scope of the scan. The host in the URL will be
  // whitelisted and the URL itself is scheduled for fetching.
  // Returns True on success.
  bool AddURL(const std::string& url);
  // Load URLs from the given file.
  // Returns True on success.
  bool LoadURLFile(const std::string& url_file);
  // Initialize the reporting classes using the report types string. This
  // returns True on success and false on failure.
  bool InitReporting(const std::string& report_types);
  // Set the shutdown timestamp. When this time is reached, plusfish will
  // gracefully shutdown.
  void SetShutdownTime(int64 time_ms);
  // Graceful shutdown handler. Calling this will cause plusfish to stop
  // scheduling new requests on the HTTP client. Ongoing requests are still
  // going to be handled.
  void ShutdownGraceful();
  // Shutdown handler. This will cause the Plusfish instance to exit the
  // Run() loop almost instantly. Keep in mind that calling this without
  // first calling ShutdownGraceful()  will cause unfinished connections to be
  // dropped by the HTTP client.
  void Shutdown();
  // Signal handler. The first time this is called, a graceful shutdown is
  // attempted. The second time this is called, plusfish will try to exit
  // immediately (e.g. useful for pressing ^C twice).
  void SignalHandler(int sig);

 private:
  // Disables the plusfish class and will cause the Run() method to return
  // without waiting for all connection to be completed.
  void Disable();

  // The reporter instances.
  std::vector<std::unique_ptr<ReporterInterface>> reporters_;
  // The crawler instance.
  Crawler* crawler_;
  // The internal clock;
  Clock* clock_;
  // The selective auditor instance which is used to perform security tests.
  SelectiveAuditor* selective_auditor_;
  // Max amount security check runners allowed to run simultaneously.
  int max_auditor_runners_;
  // The http client.
  HttpClientInterface* http_client_;
  // The datastore which contains the site tree and all detected security
  // issues.
  DataStore* datastore_;
  // The 404 fingerprinter
  NotFoundDetector* not_found_detector_;
  // The hidden objects finder
  HiddenObjectsFinder* objects_finder_;
  // A boolean which indicates whether the main plusfish process should
  // keep running. If switched to true, the polling loop will exit.
  bool enabled_ ABSL_GUARDED_BY(enabled_mutex_);
  // Mutex for the enabled flag.
  absl::Mutex enabled_mutex_;
  // A timestamp indicating when plusfish has to exit.
  int64 shutdown_time_ms_ ABSL_GUARDED_BY(shutdown_time_mutex_);
  // Mutex for the shutdown timestamp.
  absl::Mutex shutdown_time_mutex_;
  DISALLOW_COPY_AND_ASSIGN(Plusfish);
};

}  // namespace plusfish

#endif  // PLUSFISH_PLUSFISH_H_
