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

#include "audit/passive_auditor.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "proto/security_check.pb.h"
#include "response.h"
#include "testing/matcher_factory_mock.h"
#include "testing/matcher_mock.h"
#include "testing/request_mock.h"

using ::testing::_;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::ReturnNull;

namespace plusfish {

class PassiveAuditorTest : public ::testing::Test {
 protected:
  PassiveAuditorTest() {
    mock_request_.reset(new testing::MockRequest("http://example.org/"));
    // Add a basic rule to cover several test cases.
    MatchRule* rule = security_test_.mutable_matching_rule();
    MatchRule_Condition* condition = rule->add_condition();
    condition->set_target(MatchRule_Target_RESPONSE_BODY);
    MatchRule_Match* match = condition->add_match();
    match->add_value("foo");
    match->set_method(MatchRule_Method_CONTAINS);
  }

  SecurityTest security_test_;
  testing::MockMatcherFactory mock_factory_;
  testing::MockMatcher* mock_matcher_;
  std::unique_ptr<testing::MockRequest> mock_request_;
};

TEST_F(PassiveAuditorTest, PassiveAuditorAddsOk) {
  mock_matcher_ = new testing::MockMatcher();
  EXPECT_CALL(mock_factory_, GetMatcher(_)).WillOnce(Return(mock_matcher_));
  EXPECT_CALL(*mock_matcher_, Prepare()).WillOnce(Return(true));
  PassiveAuditor auditor(&mock_factory_);
  EXPECT_TRUE(auditor.AddSecurityTest(security_test_));
  EXPECT_EQ(1, auditor.response_matcher_count());
}

TEST_F(PassiveAuditorTest, PassiveAuditorFailsInFactory) {
  EXPECT_CALL(mock_factory_, GetMatcher(_)).WillOnce(ReturnNull());
  PassiveAuditor auditor(&mock_factory_);
  EXPECT_FALSE(auditor.AddSecurityTest(security_test_));
  EXPECT_EQ(0, auditor.response_matcher_count());
}

TEST_F(PassiveAuditorTest, PassiveAuditorCheckEmptyRequest) {
  EXPECT_CALL(*mock_request_, response()).WillOnce(ReturnNull());
  PassiveAuditor auditor(&mock_factory_);
  EXPECT_FALSE(auditor.Check(mock_request_.get()));
}

TEST_F(PassiveAuditorTest, PassiveAuditorCheckFindsNothing) {
  Response response;
  response.Parse("HTTP/1.0 200 OK\r\n\r\n");

  // Initialize OK.
  mock_matcher_ = new testing::MockMatcher();
  EXPECT_CALL(mock_factory_, GetMatcher(_)).WillOnce(Return(mock_matcher_));
  EXPECT_CALL(*mock_matcher_, Prepare()).WillOnce(Return(true));
  EXPECT_CALL(*mock_matcher_, MatchAny(NotNull(), NotNull()))
      .WillOnce(Return(false));

  EXPECT_CALL(*mock_request_, response())
      .Times(2)
      .WillRepeatedly(Return(&response));

  PassiveAuditor auditor(&mock_factory_);
  EXPECT_TRUE(auditor.AddSecurityTest(security_test_));
  ASSERT_EQ(1, auditor.response_matcher_count());
  EXPECT_TRUE(auditor.Check(mock_request_.get()));
}

TEST_F(PassiveAuditorTest, PassiveAuditorCheckFindsIssue) {
  Response response;
  response.Parse("HTTP/1.0 200 OK\r\n\r\n");

  // Initialize OK.
  mock_matcher_ = new testing::MockMatcher();
  EXPECT_CALL(mock_factory_, GetMatcher(_)).WillOnce(Return(mock_matcher_));
  EXPECT_CALL(*mock_matcher_, Prepare()).WillOnce(Return(true));
  EXPECT_CALL(*mock_matcher_, MatchAny(NotNull(), NotNull()))
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_request_, response())
      .Times(2)
      .WillRepeatedly(Return(&response));

  PassiveAuditor auditor(&mock_factory_);

  bool callback_called = false;
  auditor.SetRegisterIssueCallback(
      [&callback_called](const int64, const IssueDetails::IssueType,
                         const Severity, const Request*) {
        callback_called = true;
        return callback_called;
      });
  EXPECT_TRUE(auditor.AddSecurityTest(security_test_));
  ASSERT_EQ(1, auditor.response_matcher_count());
  EXPECT_TRUE(auditor.Check(mock_request_.get()));
  EXPECT_TRUE(callback_called);
}

}  // namespace plusfish
