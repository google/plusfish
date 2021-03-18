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

#ifndef PLUSFISH_AUDIT_PASSIVE_AUDITOR_H_
#define PLUSFISH_AUDIT_PASSIVE_AUDITOR_H_

#include <memory>
#include <unordered_map>

#include "base/macros.h"
#include "audit/generic_response_matcher.h"
#include "audit/matchers/matcher_factory.h"
#include "proto/issue_details.pb.h"
#include "proto/security_check.pb.h"
#include "request.h"

namespace plusfish {

// A passive auditor that reviews Request instances for security issues.
// Once loaded with one of more SecurityTest's; the auditor can be used on all
// security tests.
//
// Usage:
//   PassiveAuditor auditor(matcher_factory);
//   auditor.AddSecurityTest(test_with_match_rule);
//   auditor.AddSecurityTest(another_test_with_match_rule);
//   auditor.SetRegisterIssueCallback(issue_handling_callback);
//   auditor.Check(completed_request);
class PassiveAuditor {
 public:
  // Does not take ownership of the matcher factory.
  explicit PassiveAuditor(const MatcherFactory* matcher_factory);
  virtual ~PassiveAuditor();

  // Returns the amount of response matchers contained.
  const int response_matcher_count() const { return response_matchers_.size(); }

  // Add a SecurityTest to the auditor.
  // The SecurityTest instance is expected to have a 'matching_rule' that is
  // validated.
  // Returns true on success and false on failure (e.g. a security check
  // definition is missing a required field).
  bool AddSecurityTest(const SecurityTest& check);

  // Apply the passive checks to the given request.
  // Returns false if the request could not be checked. Else true is returned.
  // Does not take ownership.
  bool Check(Request* request) const;

  // Set's a callback that is used to register newly detected issues.
  void SetRegisterIssueCallback(
      std::function<bool(const int64 request_id,
                         const IssueDetails::IssueType issue_type,
                         const Severity severity, const Request* test_request)>
          callback);

 protected:
  PassiveAuditor() : matcher_factory_(nullptr) {}

 private:
  // The matcher factory which is used by the response matchers.
  const MatcherFactory* matcher_factory_;
  // Stores response matchers with their security test.
  std::map<std::unique_ptr<GenericResponseMatcher>, const SecurityTest>
      response_matchers_;
  // The register issue callback.
  std::function<bool(const int64 request_id,
                     const IssueDetails::IssueType issue_type,
                     const Severity severity, const Request* test_request)>
      issue_callback_;
  DISALLOW_COPY_AND_ASSIGN(PassiveAuditor);
};

}  // namespace plusfish

#endif  // PLUSFISH_AUDIT_PASSIVE_AUDITOR_H_
