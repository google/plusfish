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

#ifndef PLUSFISH_AUDIT_GENERIC_RESPONSE_MATCHER_H_
#define PLUSFISH_AUDIT_GENERIC_RESPONSE_MATCHER_H_

#include <map>

#include "opensource/deps/base/macros.h"
#include "opensource/deps/base/port.h"
#include "audit/matchers/condition_matcher.h"
#include "audit/matchers/matcher.h"
#include "audit/matchers/matcher_factory.h"
#include "audit/response_matcher.h"
#include "proto/matching.pb.h"

namespace plusfish {

class Request;

// This response matcher can be used to search strings many parts of the
// request. While it allows you to narrow down search locations; this matcher
// does not perform context aware searches. E.g. it case if the std::string is
// in a text file, a JavaScript section or an HTML comments.
//
// Usage:
//   GenericResponseMatcher matcher(match_rule);
//   if (matcher.Init()) {
//     if(matcher.Match(request_with_response)) {
//       process_successful_match(..);
//     }
//   }
class GenericResponseMatcher : public ResponseMatcherInterface {
 public:
  // Create a new instance with a ResponseMatching message.
  GenericResponseMatcher(const MatchRule& match_rule,
                         const MatcherFactory* matcher_factory);
  ~GenericResponseMatcher() override;

  // Initalizes the matcher and needs to be called before using the Match
  // methods. Returns false if the matcher could not initialize. Else true is
  // returned.
  bool Init() override MUST_USE_RESULT;
  // Match the rules against the responses of multiple requests. Will
  // return true if one or more match. Else false is returned.
  bool Match(const std::vector<std::unique_ptr<Request>>* requests) override;
  // Same as above but only for one request.
  // Does not take ownership.
  bool MatchSingle(const Request* req) override;

 private:
  // The response matching rule.
  const MatchRule match_rule_;
  // The Matcher factory which is used to get Matcher's.
  const MatcherFactory* matcher_factory_;
  // The condition matcher.
  std::map<std::unique_ptr<ConditionMatcher>, MatchRule_Condition> matchers_;
  DISALLOW_COPY_AND_ASSIGN(GenericResponseMatcher);
};

}  // namespace plusfish

#endif  // PLUSFISH_AUDIT_GENERIC_RESPONSE_MATCHER_H_
