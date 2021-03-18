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

#ifndef PLUSFISH_TESTING_SECURITY_CHECK_MOCK_H_
#define PLUSFISH_TESTING_SECURITY_CHECK_MOCK_H_

#include "audit/security_check.h"

#include <functional>
#include <memory>
#include <vector>

#include "gmock/gmock.h"
#include "proto/issue_details.pb.h"

namespace plusfish {
class Request;

namespace testing {

class MockSecurityCheck : public SecurityCheckInterface {
 public:
  MOCK_CONST_METHOD0(name, const std::string&());
  MOCK_CONST_METHOD0(issue_type, const IssueDetails::IssueType());
  MOCK_CONST_METHOD0(severity, const Severity());
  MOCK_CONST_METHOD0(CanEvaluateInSerial, bool());
  MOCK_METHOD0(next, SecurityCheckInterface*());
  MOCK_METHOD1(set_next, void(SecurityCheckInterface* check));
  MOCK_METHOD2(CreateRequests,
               bool(const Request& req,
                    std::vector<std::unique_ptr<Request>>* requests));
  MOCK_METHOD1(Evaluate,
               bool(const std::vector<std::unique_ptr<Request>>& requests));
  MOCK_METHOD1(EvaluateSingle, bool(const Request* request));
  MOCK_METHOD1(
      SetRequestMetaCallback,
      void(std::function<bool(const int64 request_id, const MetaData_Type type,
                              const int64 value)>
               callback));
  MOCK_METHOD1(SetGetRequestMetaCallback,
               void(std::function<bool(const int64 request_id,
                                       const MetaData_Type type, int64* value)>
                        callback));
};

}  // namespace testing
}  // namespace plusfish

#endif  // PLUSFISH_TESTING_SECURITY_CHECK_MOCK_H_
