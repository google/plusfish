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

#include "audit/matchers/contains_matcher.h"

#include <string.h>
#include <string>

#include "audit/matchers/matcher.h"
#include "proto/matching.pb.h"
#include "request.h"

namespace plusfish {

ContainsMatcher::ContainsMatcher(const MatchRule_Match& match)
    : match_(match) {}

bool ContainsMatcher::Prepare() { return !match_.value().empty(); }

bool ContainsMatcher::MatchSingle(const std::string& search_string,
                                  const std::string* content) const {
  if (match_.case_insensitive()) {
    return strcasestr(content->c_str(), search_string.c_str()) != nullptr;
  }
  return strstr(content->c_str(), search_string.c_str()) != nullptr;
}

bool ContainsMatcher::MatchAny(const Request* request,
                               const std::string* content) const {
  for (const auto& search_string : match_.value()) {
    if (MatchSingle(search_string, content)) {
      return true;
    }
  }
  return false;
}

}  // namespace plusfish
