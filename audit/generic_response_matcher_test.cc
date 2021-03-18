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

#include "audit/generic_response_matcher.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "proto/matching.pb.h"
#include "request.h"
#include "response.h"
#include "testing/matcher_factory_mock.h"
#include "testing/matcher_mock.h"
#include "testing/request_mock.h"

using ::testing::_;
using ::testing::HasSubstr;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::ReturnNull;

namespace plusfish {

class GenericResponseMatcherTest : public ::testing::Test {
 protected:
  GenericResponseMatcherTest()
      : match_string_("find_me"), mock_request_("http://example.org") {
    // Create a match rule that will cover most test cases.
    MatchRule_Condition* condition = match_rule_.add_condition();
    condition->set_target(MatchRule_Target_RESPONSE_BODY);
    MatchRule_Match* match = condition->add_match();
    match->set_method(MatchRule_Method_CONTAINS);
    match->add_value(match_string_);
  }

  const std::string match_string_;
  std::unique_ptr<GenericResponseMatcher> response_matcher_;
  MatchRule match_rule_;
  Response response_;
  testing::MockRequest mock_request_;
  testing::MockMatcherFactory mock_factory_;
};

TEST_F(GenericResponseMatcherTest, InitOk) {
  testing::MockMatcher* mock_matcher = new testing::MockMatcher();
  EXPECT_CALL(mock_factory_, GetMatcher(_)).WillOnce(Return(mock_matcher));
  EXPECT_CALL(*mock_matcher, Prepare()).WillOnce(Return(true));
  response_matcher_.reset(
      new GenericResponseMatcher(match_rule_, &mock_factory_));
  ASSERT_TRUE(response_matcher_->Init());
}

TEST_F(GenericResponseMatcherTest, InitFailsOnMatcherReturn) {
  EXPECT_CALL(mock_factory_, GetMatcher(_)).WillOnce(ReturnNull());
  response_matcher_.reset(
      new GenericResponseMatcher(match_rule_, &mock_factory_));
  ASSERT_FALSE(response_matcher_->Init());
}

TEST_F(GenericResponseMatcherTest, InitFailsOnMatcherPrepare) {
  testing::MockMatcher* mock_matcher = new testing::MockMatcher();
  EXPECT_CALL(mock_factory_, GetMatcher(_)).WillOnce(Return(mock_matcher));
  EXPECT_CALL(*mock_matcher, Prepare()).WillOnce(Return(false));
  response_matcher_.reset(
      new GenericResponseMatcher(match_rule_, &mock_factory_));
  ASSERT_FALSE(response_matcher_->Init());
}

TEST_F(GenericResponseMatcherTest, MatchBodyOk) {
  MatchRule match_rule;
  MatchRule_Condition* condition = match_rule.add_condition();
  condition->set_target(MatchRule_Target_RESPONSE_BODY);
  MatchRule_Match* match = condition->add_match();
  match->set_method(MatchRule_Method_CONTAINS);
  match->add_value(match_string_);

  std::string content_to_match("HTTP/1.0 200 OK\r\n\r\n " + match_string_);

  // The requests to match against.
  testing::MockRequest* request =
      new testing::MockRequest("http://www.example.org");
  // The response that eventually gets examined.
  EXPECT_CALL(*request, response()).WillOnce(Return(&response_));
  ASSERT_TRUE(response_.Parse(content_to_match));
  std::vector<std::unique_ptr<Request>> requests;
  requests.emplace_back(request);

  testing::MockMatcher* mock_matcher = new testing::MockMatcher();
  EXPECT_CALL(mock_factory_, GetMatcher(_)).WillOnce(Return(mock_matcher));
  EXPECT_CALL(*mock_matcher, Prepare()).WillOnce(Return(true));
  EXPECT_CALL(*mock_matcher,
              MatchAny(request, Pointee(HasSubstr(match_string_))))
      .WillOnce(Return(true));
  response_matcher_.reset(
      new GenericResponseMatcher(match_rule, &mock_factory_));
  ASSERT_TRUE(response_matcher_->Init());
  ASSERT_TRUE(response_matcher_->Match(&requests));
}

TEST_F(GenericResponseMatcherTest, MatchHeaderOk) {
  MatchRule match_rule;
  MatchRule_Condition* condition = match_rule.add_condition();
  condition->set_target(MatchRule_Target_RESPONSE_HEADER_VALUE);
  condition->set_field("Foo");
  MatchRule_Match* match = condition->add_match();
  match->set_method(MatchRule_Method_CONTAINS);
  match->add_value(match_string_);

  std::string content_to_match("HTTP/1.0 200 OK\r\nFoo: " + match_string_ +
                               "\r\n\r\n");

  // The requests to match against.
  testing::MockRequest* request =
      new testing::MockRequest("http://www.example.org");
  // The response that eventually gets examined.
  EXPECT_CALL(*request, response()).WillOnce(Return(&response_));
  ASSERT_TRUE(response_.Parse(content_to_match));
  std::vector<std::unique_ptr<Request>> requests;
  requests.emplace_back(request);

  testing::MockMatcher* mock_matcher = new testing::MockMatcher();
  EXPECT_CALL(mock_factory_, GetMatcher(_)).WillOnce(Return(mock_matcher));
  EXPECT_CALL(*mock_matcher, Prepare()).WillOnce(Return(true));
  EXPECT_CALL(*mock_matcher,
              MatchAny(request, Pointee(HasSubstr(match_string_))))
      .WillOnce(Return(true));
  response_matcher_.reset(
      new GenericResponseMatcher(match_rule, &mock_factory_));
  ASSERT_TRUE(response_matcher_->Init());
  ASSERT_TRUE(response_matcher_->Match(&requests));
}

TEST_F(GenericResponseMatcherTest, MatchHeaderWithTwoConditionsOk) {
  MatchRule match_rule;
  MatchRule_Condition* condition = match_rule.add_condition();
  condition->set_target(MatchRule_Target_RESPONSE_HEADER_VALUE);
  condition->set_field("Foo");
  MatchRule_Match* match = condition->add_match();
  match->set_method(MatchRule_Method_CONTAINS);
  match->add_value(match_string_);

  MatchRule_Condition* second_condition = match_rule.add_condition();
  *second_condition = *condition;

  std::string content_to_match("HTTP/1.0 200 OK\r\nFoo: " + match_string_ +
                               "\r\n\r\n");

  // The request/response to match against.
  testing::MockRequest* request =
      new testing::MockRequest("http://www.example.org");
  EXPECT_CALL(*request, response()).WillOnce(Return(&response_));
  ASSERT_TRUE(response_.Parse(content_to_match));
  std::vector<std::unique_ptr<Request>> requests;
  requests.emplace_back(request);

  testing::MockMatcher* mock_matcher = new testing::MockMatcher();
  testing::MockMatcher* mock_matcher2 = new testing::MockMatcher();
  EXPECT_CALL(mock_factory_, GetMatcher(_))
      .Times(2)
      .WillOnce(Return(mock_matcher))
      .WillOnce(Return(mock_matcher2));
  EXPECT_CALL(*mock_matcher, Prepare()).WillOnce(Return(true));
  EXPECT_CALL(*mock_matcher,
              MatchAny(request, Pointee(HasSubstr(match_string_))))
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_matcher2, Prepare()).WillOnce(Return(true));
  EXPECT_CALL(*mock_matcher2,
              MatchAny(request, Pointee(HasSubstr(match_string_))))
      .WillOnce(Return(true));

  response_matcher_.reset(
      new GenericResponseMatcher(match_rule, &mock_factory_));
  ASSERT_TRUE(response_matcher_->Init());
  ASSERT_TRUE(response_matcher_->Match(&requests));
}

TEST_F(GenericResponseMatcherTest, MatchFailsOnMissingResponse) {
  // The requests to match against.
  testing::MockRequest* request =
      new testing::MockRequest("http://www.example.org");
  EXPECT_CALL(*request, response()).WillOnce(ReturnNull());
  std::vector<std::unique_ptr<Request>> requests;
  requests.emplace_back(request);

  testing::MockMatcher* mock_matcher = new testing::MockMatcher();
  EXPECT_CALL(mock_factory_, GetMatcher(_)).WillOnce(Return(mock_matcher));
  EXPECT_CALL(*mock_matcher, Prepare()).WillOnce(Return(true));
  response_matcher_.reset(
      new GenericResponseMatcher(match_rule_, &mock_factory_));
  ASSERT_TRUE(response_matcher_->Init());
  ASSERT_FALSE(response_matcher_->Match(&requests));
}

}  // namespace plusfish
