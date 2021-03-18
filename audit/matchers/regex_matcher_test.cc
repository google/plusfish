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
#include "gtest/gtest.h"
#include "proto/matching.pb.h"
#include "testing/request_mock.h"

namespace plusfish {

class RegexMatcherTest : public ::testing::Test {
 protected:
  RegexMatcherTest()
      : content_to_match_("test"), mock_request_("http://example.org") {}
  MatchRule_Match match_;
  std::string content_to_match_;
  testing::MockRequest mock_request_;
};

TEST_F(RegexMatcherTest, MatchesOk) {
  match_.add_value("\\w+");
  RegexMatcher matcher(match_);
  ASSERT_TRUE(matcher.Prepare());
  ASSERT_TRUE(matcher.MatchAny(&mock_request_, &content_to_match_));
}

TEST_F(RegexMatcherTest, MatchAnyWithMultipleValuesOk) {
  match_.add_value("[A-Z]+");
  match_.add_value("[a-z]+");
  match_.add_value("[0-9]+");
  RegexMatcher matcher(match_);
  ASSERT_TRUE(matcher.Prepare());
  ASSERT_TRUE(matcher.MatchAny(&mock_request_, &content_to_match_));
}

TEST_F(RegexMatcherTest, MatchNothing) {
  match_.add_value("[0-9]+");
  match_.add_value("is");
  match_.add_value("test");
  content_to_match_ = "nope!";
  RegexMatcher matcher(match_);
  ASSERT_TRUE(matcher.Prepare());
  ASSERT_FALSE(matcher.MatchAny(&mock_request_, &content_to_match_));
}

TEST_F(RegexMatcherTest, MatchesCaseInsensitiveOk) {
  match_.add_value("test");
  match_.set_case_insensitive(true);
  content_to_match_ = "tEsT";
  RegexMatcher matcher(match_);
  ASSERT_TRUE(matcher.Prepare());
  ASSERT_TRUE(matcher.MatchAny(&mock_request_, &content_to_match_));
}

TEST_F(RegexMatcherTest, MatchesEmptyString) {
  match_.add_value("\\w");
  RegexMatcher matcher(match_);
  content_to_match_ = "";
  ASSERT_TRUE(matcher.Prepare());
  ASSERT_FALSE(matcher.MatchAny(&mock_request_, &content_to_match_));
}

TEST_F(RegexMatcherTest, MatchesWithInvalidRegex) {
  match_.add_value("\\a[+'");
  RegexMatcher matcher(match_);
  ASSERT_FALSE(matcher.Prepare());
}

}  // namespace plusfish
