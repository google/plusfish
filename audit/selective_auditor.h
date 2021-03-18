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

#ifndef PLUSFISH_AUDIT_SELECTIVE_AUDITOR_H_
#define PLUSFISH_AUDIT_SELECTIVE_AUDITOR_H_

#include <memory>

#include "base/macros.h"
#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"
#include "audit/generic_generator.h"
#include "audit/generic_response_matcher.h"
#include "audit/generic_security_check.h"
#include "audit/matchers/matcher_factory.h"
#include "audit/security_check_runner.h"
#include "proto/issue_details.pb.h"
#include "proto/security_check.pb.h"

namespace plusfish {

class Request;

class SelectiveAuditor {
 public:
  // Does not take ownership of the matcher factory.
  SelectiveAuditor(const MatcherFactory* matcher_factory,
                   HttpClientInterface* http_client);
  virtual ~SelectiveAuditor();

  // Returns the security test runners.
  const std::vector<std::unique_ptr<SecurityCheckRunner>>& runners() const {
    return runners_;
  }

  // Returns number security check runners.
  virtual int runner_count() {
    absl::ReaderMutexLock r(&runners_mutex_);
    return runners_.size();
  }

  // Returns the security checks.
  const std::vector<std::unique_ptr<SecurityCheckInterface>>& checks() const {
    return checks_;
  }

  // This will cause the SelectiveAuditor to create a matching SecurityCheck
  // which it will use during the testing.
  // Returns true on success and false on failure (e.g. a security
  // check definition is missing a required field).
  bool AddSecurityTest(const SecurityTest& sec_test);

  // Add a security check which will be used to test all requests.
  // Takes ownership.
  void AddSecurityCheck(SecurityCheckInterface* check);

  // Schedule a request for auditing.
  // The Request must be kept in memory until the auditor is finished.
  // Returns true if security tests were scheduled.
  // Does not take ownership.
  virtual bool ScheduleFirst(const Request* request);

  // Schedule the next check in the given runner.
  // If the first check does not yield tests for the tested request (e.g. no
  // parameters to inject into) then the next check will be taken. This is done
  // until the first check is scheduled successfully or all checks have been
  // tried.
  // Returns true if a security check was scheduled.
  // Does not take ownership.
  bool ScheduleNextCheck(SecurityCheckRunner* current_runner);

  // A cleanup routing that removes a completed runner instance.
  // Already has ownership of the given runner instance. Uses the reference to
  // lookup and destroy this specific instance.
  virtual void FinishedCheckCb(SecurityCheckRunner* runner);

  // Set's a callback to the crawler. This can be used to feed Requests from
  // security tests back into the crawler.
  void SetCrawlerScrapeCallback(std::function<bool(const Request*)> callback);

  // Set's a callback that is used to register newly detected issues.
  void SetRegisterIssueCallback(
      std::function<bool(const int64 request_id,
                         const IssueDetails::IssueType issue_type,
                         const Severity severity, const Request* test_request)>
          callback);

  // Set the request metadata callback to store info into a request.
  void SetRequestMetaCallback(
      std::function<bool(const int64 request_id, const MetaData_Type type,
                         const int64 value)>
          callback);
  // Set the request metadata callback to retrieve info.
  void SetGetRequestMetaCallback(
      std::function<bool(const int64 request_id, const MetaData_Type type,
                         int64* value)>
          callback);

 protected:
  SelectiveAuditor() : matcher_factory_(nullptr) {}

 private:
  // The matcher factory which is used by the response matchers.
  const MatcherFactory* matcher_factory_;
  // Stores all the active and finished security check runners.
  std::vector<std::unique_ptr<SecurityCheckRunner>> runners_
      ABSL_GUARDED_BY(runners_mutex_);
  // Stores all the security checks.
  std::vector<std::unique_ptr<SecurityCheckInterface>> checks_;
  // Synchronizes access to the runners_ std::vector.
  absl::Mutex runners_mutex_;
  // The register issue callback.
  std::function<bool(const int64 request_id,
                     const IssueDetails::IssueType issue_type,
                     const Severity severity, const Request* test_request)>
      issue_callback_;
  // The crawler callback to scrape a request without extracting the links.
  std::function<bool(const Request*)> crawler_scrape_cb_;
  // The request metadata getter and setter callbacks.
  std::function<bool(const int64 request_id, const MetaData_Type type,
                     const int64 value)>
      set_req_meta_cb_;
  std::function<bool(const int64 request_id, const MetaData_Type type,
                     int64* value)>
      get_req_meta_cb_;
  // The HTTP client used to schedule new test requests.
  HttpClientInterface* http_client_;
  DISALLOW_COPY_AND_ASSIGN(SelectiveAuditor);
};

}  // namespace plusfish

#endif  // PLUSFISH_AUDIT_SELECTIVE_AUDITOR_H_
