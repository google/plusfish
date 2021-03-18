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

#ifndef PLUSFISH_AUDIT_SECURITY_CHECK_H_
#define PLUSFISH_AUDIT_SECURITY_CHECK_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "proto/issue_details.pb.h"
#include "request.h"

namespace plusfish {

// Interface for SecurityCheck implementations.
class SecurityCheckInterface {
 public:
  virtual ~SecurityCheckInterface() {}
  // Returns the name of the security check.
  virtual const std::string& name() const = 0;
  // Returns the next check (if any).
  virtual SecurityCheckInterface* next() = 0;
  // Set the next security check.
  virtual void set_next(SecurityCheckInterface* check) = 0;
  // Returns the type of issues this check generates.
  virtual const IssueDetails::IssueType issue_type() const = 0;
  // Returns the severity of the security check.
  virtual const Severity severity() const = 0;
  // Whether the security responses can be evaluated in serial.
  virtual bool CanEvaluateInSerial() const = 0;
  // Creates the requests for which the responses are needed to determine
  // whether 'req' is affected. The requests are owned by the caller.
  virtual bool CreateRequests(
      const Request& req, std::vector<std::unique_ptr<Request>>* requests) = 0;

  // Evaluate completed (fetched) responses and return true if they test
  // positive for the security check.
  virtual bool Evaluate(
      const std::vector<std::unique_ptr<Request>>& requests) = 0;
  // Evaluate a single request.
  virtual bool EvaluateSingle(const Request* request) = 0;
  // Set the request metadata callback to store info into a request.
  virtual void SetRequestMetaCallback(
      std::function<bool(const int64 request_id, const MetaData_Type type,
                         const int64 value)>
          callback) = 0;
  // Set the request metadata callback to retrieve info.
  virtual void SetGetRequestMetaCallback(
      std::function<bool(const int64 request_id, const MetaData_Type type,
                         int64* value)>
          callback) = 0;

 protected:
  SecurityCheckInterface() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SecurityCheckInterface);
};

}  // namespace plusfish

#endif  // PLUSFISH_AUDIT_SECURITY_CHECK_H_
