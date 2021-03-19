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

#ifndef PLUSFISH_AUDIT_MATCHERS_REGEX_MATCHER_H_
#define PLUSFISH_AUDIT_MATCHERS_REGEX_MATCHER_H_

#include <map>
#include <memory>

#include "opensource/deps/base/macros.h"
#include "re2/re2.h"
#include "audit/matchers/matcher.h"
#include "proto/matching.pb.h"
#include "request.h"

namespace plusfish {

class RegexMatcher : public MatcherInterface {
 public:
  explicit RegexMatcher(const MatchRule_Match& match);
  ~RegexMatcher() override {}

  // If negative matching is expected.
  bool negative() const override { return match_.negative_match(); }
  // Compiles the regular expression(s) and returns false when there are errors.
  // If a match has to be case insensitive, the regular expression(s) will be
  // be prefixed with (?i) when this is missing.
  bool Prepare() override MUST_USE_RESULT;

  // Try all regular expressions against the given content. Returns true on the
  // first successful match. If nothing matches, false is returned.
  // The Prepare method MUST be called before calling this.
  bool MatchAny(const Request* request,
                const std::string* content) const override;

 private:
  // Whether the Prepared method has been called.
  bool prepared_;
  // The match rule.
  const MatchRule_Match match_;
  // The compiled regular expressions.
  std::vector<std::unique_ptr<RE2>> match_res_;
  DISALLOW_COPY_AND_ASSIGN(RegexMatcher);
};

}  // namespace plusfish

#endif  // PLUSFISH_AUDIT_MATCHERS_REGEX_MATCHER_H_
