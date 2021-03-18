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

#ifndef PLUSFISH_AUDIT_SECURITY_CHECK_RUNNER_H_
#define PLUSFISH_AUDIT_SECURITY_CHECK_RUNNER_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "audit/security_check.h"
#include "proto/issue_details.pb.h"
#include "request_handler.h"

namespace plusfish {

class HttpClientInterface;
class Request;

// A helper class to run a security check against a single Request instance.
// Instances of this class will only live as long as needed to complete
// a single security test for a single Request.
//
// Usage:
//   Request req("http://example.org/foo?bar=1");
//   HttpClient http_client()
//   SecurityCheck check(rule, &req);
//   SecurityCheckRunner runner(&check, &http_client);
//   runner.Run(&req);
class SecurityCheckRunner : public RequestHandlerInterface {
 public:
  // Create an runner for SecurityCheck 'check'. A pointer to the original
  // request is stored and used for reporting any detected issues.
  // Does not take ownership.
  SecurityCheckRunner(SecurityCheckInterface* check, const Request* request);
  ~SecurityCheckRunner() override {}

  // Accessor to the finished bool. This indicates whether the runner is done
  // and can be cleaned up.
  bool finished() const { return finished_; }

  // Return the Request that is currently being tested.
  const Request* tested_request() const { return tested_request_; }

  // Returns the contained security check name.
  const std::string& check_name() const { return security_check_->name(); }

  // Returns the number of requests that were completed.
  const int requests_completed() const { return requests_completed_; }

  // Returns the security check Requests.
  const std::vector<std::unique_ptr<Request>>& requests() const {
    return requests_;
  }

  // Perform the security check for the tested_request by scheduling the
  // SecurityTest generated request for fetching. Returns true when
  // requests where scheduled, else false.
  // Does not take ownership.
  bool Run(HttpClientInterface* http_client);

  // Move on to the next security check and reset the state.
  // Returns false when the current check has no reference to another (next)
  // check. Else true is returned.
  virtual bool SetNextCheck();

  // Callback for completed requests.
  // The HTTP client will call this callback whenever a request has received a
  // response. Does not take ownership.
  int RequestCallback(Request* req) override;

  // Register a callback that is called when a security check is finished.
  void OnCheckDone(std::function<void(SecurityCheckRunner*)> callback);

  // Set's a callback that is used to register newly detected issues.
  void SetRegisterIssueCallback(
      std::function<bool(const int64 request_id,
                         const IssueDetails::IssueType issue_type,
                         const Severity severity, const Request* test_request)>
          callback);

 private:
  // A bool indicating whether the runner is finished.
  bool finished_;
  // Request for which a callback was received.
  int requests_completed_;
  // The base Request instance.
  const Request* tested_request_;
  // Holds the security check instance.
  SecurityCheckInterface* security_check_;
  // Holds the HTTP requests for this test;
  std::vector<std::unique_ptr<Request>> requests_;
  // Callback method which is called when the runner has completed its work and
  // can be destroyed.
  std::function<void(SecurityCheckRunner*)> check_done_callback_;
  // The register issue callback.
  std::function<bool(const int64 request_id,
                     const IssueDetails::IssueType issue_type,
                     const Severity severity, const Request* test_request)>
      issue_callback_;

  DISALLOW_COPY_AND_ASSIGN(SecurityCheckRunner);
};

}  // namespace plusfish
#endif  // PLUSFISH_AUDIT_SECURITY_CHECK_RUNNER_H_
