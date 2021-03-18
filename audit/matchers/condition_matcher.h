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

#ifndef PLUSFISH_AUDIT_MATCHERS_CONDITION_MATCHER_H_
#define PLUSFISH_AUDIT_MATCHERS_CONDITION_MATCHER_H_

#include "base/macros.h"
#include "audit/matchers/matcher.h"
#include "proto/matching.pb.h"
#include "request.h"

namespace plusfish {

// The condition matcher can hold several unique Matcher instances and rule^H^H
// apply them all at once.
//
// Usage:
//   ConditionMatcher condition;
//   EqualsMatcher em = new EqualsMatcher(config));
//   em.Prepare()
//   ContainsMatcher cm = new ContainsMatcher(config));
//   cm.Prepare()
//   condition.AddMatcher(em);
//   condition.AddMatcher(cm);
//   if (condition.Match(content)) {
//     // do something
//   }
class ConditionMatcher {
 public:
  ConditionMatcher() {}
  ~ConditionMatcher() {}

  // Add a matcher. The matchers that are added have to already been prepared.
  // Takes ownership of the matcher.
  void AddMatcher(MatcherInterface* prepared_matcher);

  // Returns true when all of the contained matcher returned true.
  // The request should be the request that resulted in the given content.
  // Else false is returned.
  bool Match(const Request* request, const std::string* content) const;

 private:
  std::vector<std::unique_ptr<MatcherInterface>> matchers_;
  DISALLOW_COPY_AND_ASSIGN(ConditionMatcher);
};

}  // namespace plusfish

#endif  // PLUSFISH_AUDIT_MATCHERS_CONDITION_MATCHER_H_
