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

#ifndef PLUSFISH_AUDIT_RESPONSE_TIME_CHECK_H_
#define PLUSFISH_AUDIT_RESPONSE_TIME_CHECK_H_

#include "base/macros.h"
#include "audit/security_check.h"
#include "proto/issue_details.pb.h"
#include "proto/severity.pb.h"
#include "request.h"

namespace plusfish {

// Measures and evaluates the average response time of a request.
// Also stores the average response time in the request metadata.
//
// Usage:
//   ResponseTimeCheck check(number_request_to_create,
//                           max_acceptable_response_time_ms);
//   std::vector<std::unique_ptr<Request>> requests;
//   check.CreateRequests(request_to_check, &requests);
//   crawler.Fetch(requests);
//   if (check.Evaluate(requests)) {
//     average_response_too_slow = true;
//   }
class ResponseTimeCheck : public SecurityCheckInterface {
 public:
  // The number of test requests parameter controls how many copies of the
  // original requests will be on the HTTP client.
  // The max response time specifies the point at which the server time is no
  // longer acceptable.
  ResponseTimeCheck(int number_requests_to_create,
                    int max_average_response_time_ms);
  ~ResponseTimeCheck() override;

  const std::string& name() const override { return name_; }

  // Returns the next security check or nullptr.
  SecurityCheckInterface* next() override { return next_check_; }

  // Set the next security check.
  void set_next(SecurityCheckInterface* check) override { next_check_ = check; }

  // This check only reports very slow servers.
  const IssueDetails::IssueType issue_type() const override {
    return IssueDetails::SLOW_SERVER;
  }

  const Severity severity() const override { return Severity::MINIMAL; }

  bool CanEvaluateInSerial() const override { return false; }

  // Fills the 'requests' std::vector with (unmodified) copies of 'req'.
  // Does not take ownership.
  bool CreateRequests(const Request& req,
                      std::vector<std::unique_ptr<Request>>* requests) override;

  // Calculates the average application response time using the given requests.
  // The average response time is then stored in the metadata for the request
  // under test.
  // Returns true if the average response time exceeds our maximum. Else false
  // is returned.
  bool Evaluate(const std::vector<std::unique_ptr<Request>>& requests) override;

  // Will by default return false as this class requires non-serial evaluation.
  bool EvaluateSingle(const Request* request) override;

  // Set the request metadata callback to store info into a request.
  void SetRequestMetaCallback(
      std::function<bool(const int64 request_id, const MetaData_Type type,
                         const int64 value)>
          callback) override;
  // Set the request metadata callback to retrieve info.
  void SetGetRequestMetaCallback(
      std::function<bool(const int64 request_id, const MetaData_Type type,
                         int64* value)>
          callback) override;

 private:
  // The number of requests to schedule.
  int number_requests_to_create_;
  // The application response time theshold.
  int max_average_response_time_ms_;
  // The next security check.
  SecurityCheckInterface* next_check_;
  // The name of this check.
  const std::string name_;
  // The request metadata getter and setter callbacks.
  std::function<bool(const int64 request_id, const MetaData_Type type,
                     const int64 value)>
      set_req_meta_cb_;
  std::function<bool(const int64 request_id, const MetaData_Type type,
                     int64* value)>
      get_req_meta_cb_;
  DISALLOW_COPY_AND_ASSIGN(ResponseTimeCheck);
};

}  // namespace plusfish
#endif  // PLUSFISH_AUDIT_RESPONSE_TIME_CHECK_H_
