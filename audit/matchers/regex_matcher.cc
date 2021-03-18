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

#include "audit/matchers/regex_matcher.h"

#include <string>

#include "re2/re2.h"
#include "audit/matchers/matcher.h"
#include "proto/matching.pb.h"
#include "request.h"

namespace plusfish {

// This regular expression prefix enables case insensitive matching.
static const char* kCaseInsensitivePrefix = "(?i)";

RegexMatcher::RegexMatcher(const MatchRule_Match& match)
    : prepared_(false), match_(match) {}

bool RegexMatcher::Prepare() {
  prepared_ = true;
  for (const std::string& re_string : match_.value()) {
    if (match_.case_insensitive() &&
        re_string.substr(0, 4) != kCaseInsensitivePrefix) {
      match_res_.push_back(
          std::unique_ptr<RE2>(new RE2(kCaseInsensitivePrefix + re_string)));
    } else {
      match_res_.push_back(std::unique_ptr<RE2>(new RE2(re_string)));
    }

    if (!match_res_.back()->ok()) {
      return false;
    }
  }
  return true;
}

bool RegexMatcher::MatchAny(const Request* request,
                            const std::string* content) const {
  for (const auto& re : match_res_) {
    if (RE2::PartialMatch(*content, *re)) {
      return true;
    }
  }
  return false;
}

}  // namespace plusfish
