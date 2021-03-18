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

#ifndef PLUSFISH_AUDIT_MATCHERS_CONTAINS_MATCHER_H_
#define PLUSFISH_AUDIT_MATCHERS_CONTAINS_MATCHER_H_

#include "base/macros.h"
#include "audit/matchers/matcher.h"
#include "proto/matching.pb.h"
#include "request.h"

namespace plusfish {

// Can be used to check if one std::string contains a second string.
class ContainsMatcher : public MatcherInterface {
 public:
  explicit ContainsMatcher(const MatchRule_Match& match);
  ~ContainsMatcher() override {}

  // If negative matching is expected.
  bool negative() const override { return match_.negative_match(); }
  bool Prepare() override;
  // Returns true if any of the contained values match the Request and it's
  // content. Else false is returned.
  bool MatchAny(const Request* request,
                const std::string* content) const override;

 private:
  // Helper method to match a single std::string against the content and it's
  // origin Request. Returns true on success and false on failure.
  bool MatchSingle(const std::string& search_string,
                   const std::string* content) const;
  const MatchRule_Match match_;
  DISALLOW_COPY_AND_ASSIGN(ContainsMatcher);
};

}  // namespace plusfish

#endif  // PLUSFISH_AUDIT_MATCHERS_CONTAINS_MATCHER_H_
