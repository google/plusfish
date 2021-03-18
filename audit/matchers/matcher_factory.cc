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

#include "audit/matchers/matcher_factory.h"

#include "audit/matchers/contains_matcher.h"
#include "audit/matchers/equals_matcher.h"
#include "audit/matchers/prefix_matcher.h"
#include "audit/matchers/regex_matcher.h"
#include "audit/matchers/timing_matcher.h"

namespace plusfish {

MatcherFactory::MatcherFactory() {}

MatcherInterface* MatcherFactory::GetMatcher(
    const MatchRule_Match& match) const {
  switch (match.method()) {
    case MatchRule_Method_CONTAINS:
      return new ContainsMatcher(match);
    case MatchRule_Method_REGEX:
      return new RegexMatcher(match);
    case MatchRule_Method_PREFIX:
      return new PrefixMatcher(match);
    case MatchRule_Method_EQUALS:
      return new EqualsMatcher(match);
    case MatchRule_Method_TIMING:
      return new TimingMatcher(match, get_req_meta_cb_);
    case MatchRule_Method_NONE:
      break;
  }
  return nullptr;
}

}  // namespace plusfish
