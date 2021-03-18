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
#include "gtest/gtest.h"
#include "proto/matching.pb.h"
#include "testing/request_mock.h"

namespace plusfish {

class ContainsMatcherTest : public ::testing::Test {
 protected:
  ContainsMatcherTest()
      : content_to_match_("this is a test"),
        mock_request_("http://example.org") {}
  MatchRule_Match match_;
  std::string content_to_match_;
  testing::MockRequest mock_request_;
};

TEST_F(ContainsMatcherTest, MatchesOk) {
  match_.add_value("test");
  ContainsMatcher matcher(match_);
  ASSERT_TRUE(matcher.MatchAny(&mock_request_, &content_to_match_));
}

TEST_F(ContainsMatcherTest, MatchAnyWithMultipleValuesOk) {
  match_.add_value("this");
  match_.add_value("is");
  match_.add_value("test");
  ContainsMatcher matcher(match_);
  ASSERT_TRUE(matcher.MatchAny(&mock_request_, &content_to_match_));
}

TEST_F(ContainsMatcherTest, MatchNothing) {
  match_.add_value("does");
  match_.add_value("not");
  match_.add_value("match");
  ContainsMatcher matcher(match_);
  ASSERT_FALSE(matcher.MatchAny(&mock_request_, &content_to_match_));
}

TEST_F(ContainsMatcherTest, MatchesCaseInsensitiveOk) {
  match_.add_value("test");
  match_.set_case_insensitive(true);
  content_to_match_ = "tHIS Is a tEsT !";
  ContainsMatcher matcher(match_);
  ASSERT_TRUE(matcher.MatchAny(&mock_request_, &content_to_match_));
}

TEST_F(ContainsMatcherTest, MatchesEmptyString) {
  match_.add_value("test");
  content_to_match_ = "";
  ContainsMatcher matcher(match_);
  ASSERT_FALSE(matcher.MatchAny(&mock_request_, &content_to_match_));
}

}  // namespace plusfish
