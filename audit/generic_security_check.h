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

#ifndef PLUSFISH_AUDIT_GENERIC_SECURITY_CHECK_H_
#define PLUSFISH_AUDIT_GENERIC_SECURITY_CHECK_H_

#include "base/macros.h"
#include "audit/generator.h"
#include "audit/response_matcher.h"
#include "audit/security_check.h"
#include "proto/issue_details.pb.h"
#include "proto/security_check.pb.h"
#include "request.h"

namespace plusfish {
// Represents a single security check (e.g. directory traversal).
//
// The GenericSecurityCheck instances hold logic to generate requests
// and evaluate responses in order to determine whether a URL/Request
// is vulnerable. Best used with a SecurityTestRunner.
//
// Usage:
//   GenericSecurityCheck check(generator, matcher, security_test_proto);
//   std::vector<std::unique_ptr<Request>> requests;
//   check.CreateRequests(request_to_check, &requests);
//   crawler.Fetch(requests);
//   check.Evaluate(requests);
class GenericSecurityCheck : public SecurityCheckInterface {
 public:
  // Create an instance. The generator and matcher instances have to be
  // validated before given to this class.
  // Takes ownership of the generator and matcher.
  GenericSecurityCheck(GeneratorInterface* generator,
                       ResponseMatcherInterface* matcher,
                       const SecurityTest& test);

  ~GenericSecurityCheck() override;

  // Returns the security test name from the embedded proto.
  const std::string& name() const override { return security_test_.name(); }

  // Return the next security check or nullptr.
  SecurityCheckInterface* next() override { return next_check_; }

  // Set the next security check.
  void set_next(SecurityCheckInterface* check) override { next_check_ = check; }

  // Returns the type of issues that this check will generate.
  const IssueDetails::IssueType issue_type() const override {
    return security_test_.issue_type();
  }

  // Returns the security test severity from the embedded proto.
  const Severity severity() const override {
    return security_test_.advisory().severity();
  }

  // Returns true if responses can be evaluated in serial (one by one). Else
  // false is returned.
  bool CanEvaluateInSerial() const override {
    return security_test_.allow_serial_evaluation();
  }

  // Set the request metadata callback to store info into a request.
  void SetRequestMetaCallback(
      std::function<bool(const int64 request_id, const MetaData_Type type,
                         const int64 value)>
          callback) override {
    // Currently unused.
  }

  // Set the request metadata callback to retrieve info.
  void SetGetRequestMetaCallback(
      std::function<bool(const int64 request_id, const MetaData_Type type,
                         int64* value)>
          callback) override {
    // Currently unused.
  }

  // Create request instances that have to be scheduled for this request. The
  // caller will own the requests. Returns false on failure, true on success.
  // Does not take ownership.
  bool CreateRequests(const Request& req,
                      std::vector<std::unique_ptr<Request>>* requests) override;

  // Evaluate the responses by using the evaluation rules contained in
  // the SecurityTest. Returns true if the rules matched. Else false.
  bool Evaluate(const std::vector<std::unique_ptr<Request>>& requests) override;

  // Same as above but for a single request.
  bool EvaluateSingle(const Request* request) override;

 private:
  // Holds the definition of the security test.
  const SecurityTest security_test_;
  // Holds a pointer to the security check that will be scheduled next.
  SecurityCheckInterface* next_check_;
  // The request generator.
  std::unique_ptr<GeneratorInterface> generator_;
  std::unique_ptr<ResponseMatcherInterface> matcher_;
  DISALLOW_COPY_AND_ASSIGN(GenericSecurityCheck);
};

}  // namespace plusfish
#endif  // PLUSFISH_AUDIT_GENERIC_SECURITY_CHECK_H_
