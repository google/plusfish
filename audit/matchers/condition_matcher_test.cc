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

#include "audit/matchers/condition_matcher.h"

#include <memory>

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "testing/matcher_mock.h"
#include "testing/request_mock.h"

using testing::Return;

namespace plusfish {

class ConditionMatcherTest : public ::testing::Test {
 protected:
  ConditionMatcherTest()
      : content_to_match_("test"), mock_request_("http://example.org") {}
  const std::string content_to_match_;
  ConditionMatcher matcher_;
  testing::MockRequest mock_request_;
};

TEST_F(ConditionMatcherTest, MatchesAllOk) {
  testing::MockMatcher* mock_matcher = new testing::MockMatcher();
  testing::MockMatcher* second_mock_matcher = new testing::MockMatcher();
  EXPECT_CALL(*second_mock_matcher,
              MatchAny(&mock_request_, &content_to_match_))
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_matcher, MatchAny(&mock_request_, &content_to_match_))
      .WillOnce(Return(true));
  matcher_.AddMatcher(mock_matcher);
  matcher_.AddMatcher(second_mock_matcher);
  ASSERT_TRUE(matcher_.Match(&mock_request_, &content_to_match_));
}

TEST_F(ConditionMatcherTest, MatchesAllWithOneNegativeMatcherOk) {
  testing::MockMatcher* mock_matcher = new testing::MockMatcher();
  testing::MockMatcher* negative_matcher = new testing::MockMatcher();

  EXPECT_CALL(*mock_matcher, MatchAny(&mock_request_, &content_to_match_))
      .WillOnce(Return(true));
  EXPECT_CALL(*negative_matcher, MatchAny(&mock_request_, &content_to_match_))
      .WillOnce(Return(false));
  EXPECT_CALL(*negative_matcher, negative()).WillOnce(Return(true));

  matcher_.AddMatcher(mock_matcher);
  matcher_.AddMatcher(negative_matcher);
  ASSERT_TRUE(matcher_.Match(&mock_request_, &content_to_match_));
}

TEST_F(ConditionMatcherTest, MatchesAllReturnsFalse) {
  testing::MockMatcher* mock_matcher = new testing::MockMatcher();
  testing::MockMatcher* second_mock_matcher = new testing::MockMatcher();
  EXPECT_CALL(*mock_matcher, MatchAny(&mock_request_, &content_to_match_))
      .WillOnce(Return(false));
  matcher_.AddMatcher(mock_matcher);
  matcher_.AddMatcher(second_mock_matcher);
  ASSERT_FALSE(matcher_.Match(&mock_request_, &content_to_match_));
}

TEST_F(ConditionMatcherTest, MatchesWithoutContent) {
  testing::MockMatcher* mock_matcher = new testing::MockMatcher();
  EXPECT_CALL(*mock_matcher, negative()).WillOnce(Return(true));
  matcher_.AddMatcher(mock_matcher);
  ASSERT_TRUE(matcher_.Match(&mock_request_, nullptr));
}

TEST_F(ConditionMatcherTest, MatchesNegativeWithoutContent) {
  testing::MockMatcher* mock_matcher = new testing::MockMatcher();
  EXPECT_CALL(*mock_matcher, negative()).WillOnce(Return(false));
  matcher_.AddMatcher(mock_matcher);
  ASSERT_FALSE(matcher_.Match(&mock_request_, nullptr));
}

}  // namespace plusfish
