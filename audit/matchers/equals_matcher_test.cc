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

#include "audit/matchers/equals_matcher.h"
#include "gtest/gtest.h"
#include "proto/matching.pb.h"
#include "testing/request_mock.h"

namespace plusfish {

class EqualsMatcherTest : public ::testing::Test {
 protected:
  EqualsMatcherTest()
      : content_to_match_("test"), mock_request_("http://example.org") {}
  MatchRule_Match match_;
  std::string content_to_match_;
  testing::MockRequest mock_request_;
};

TEST_F(EqualsMatcherTest, MatchAnyOk) {
  match_.add_value(content_to_match_);
  EqualsMatcher matcher(match_);
  EXPECT_TRUE(matcher.MatchAny(&mock_request_, &content_to_match_));
}

TEST_F(EqualsMatcherTest, MatchAnyWithMultipleValuesOk) {
  match_.add_value("this");
  match_.add_value("is");
  match_.add_value(content_to_match_);
  EqualsMatcher matcher(match_);
  EXPECT_TRUE(matcher.MatchAny(&mock_request_, &content_to_match_));
}

TEST_F(EqualsMatcherTest, DoesNotMatch) {
  match_.add_value("this");
  match_.add_value("is");
  match_.add_value("good");
  EqualsMatcher matcher(match_);
  EXPECT_FALSE(matcher.MatchAny(&mock_request_, &content_to_match_));
}

TEST_F(EqualsMatcherTest, MatchesCaseInsensitiveOk) {
  match_.add_value("test");
  match_.set_case_insensitive(true);
  EqualsMatcher matcher(match_);
  content_to_match_ = "tEsT";
  EXPECT_TRUE(matcher.MatchAny(&mock_request_, &content_to_match_));
}

TEST_F(EqualsMatcherTest, MatchesEmptyString) {
  match_.add_value("not empty here");
  EqualsMatcher matcher(match_);
  content_to_match_.clear();
  EXPECT_FALSE(matcher.MatchAny(&mock_request_, &content_to_match_));
}

}  // namespace plusfish
