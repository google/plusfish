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

#include "audit/selective_auditor.h"

#include <memory>

#include "gtest/gtest.h"
#include "testing/crawler_mock.h"
#include "testing/http_client_mock.h"
#include "testing/matcher_factory_mock.h"
#include "testing/matcher_mock.h"
#include "testing/request_mock.h"
#include "testing/security_check_mock.h"

using ::testing::_;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::ReturnNull;

namespace plusfish {

class SelectiveAuditorTest : public ::testing::Test {
 protected:
  SelectiveAuditorTest() {
    mock_request_.reset(new testing::MockRequest("http://example.org/?foo=aa"));
    mock_http_client_.reset(new testing::MockHttpClient());
    auditor_.reset(
        new SelectiveAuditor(&mock_factory_, mock_http_client_.get()));

    // Add a basic test to cover most test cases.
    MatchRule* match_rule = security_test_.mutable_matching_rule();
    MatchRule_Condition* condition = match_rule->add_condition();
    MatchRule_Match* match = condition->add_match();
    match->add_value("foo");
    match->set_method(MatchRule_Method_CONTAINS);
    GeneratorRule* generator = security_test_.mutable_generator_rule();
    generator->add_method(GeneratorRule_InjectionMethod_SET_VALUE);
    generator->add_encoding(GeneratorRule_EncodingType_NONE);
    generator->mutable_payload()->add_arg("string");
    GeneratorRule_PayloadTarget* target = generator->add_target();
    target->set_type(GeneratorRule_PayloadTarget_TargetType_URL_PARAMS);
  }

  SecurityTest security_test_;
  testing::MockMatcherFactory mock_factory_;
  testing::MockMatcher* mock_matcher_;
  std::unique_ptr<testing::MockRequest> mock_request_;
  std::unique_ptr<testing::MockHttpClient> mock_http_client_;
  std::unique_ptr<SelectiveAuditor> auditor_;
};

TEST_F(SelectiveAuditorTest, SelectiveAuditorAddsFromProtoOk) {
  mock_matcher_ = new testing::MockMatcher();
  EXPECT_CALL(mock_factory_, GetMatcher(_)).WillOnce(Return(mock_matcher_));
  EXPECT_CALL(*mock_matcher_, Prepare()).WillOnce(Return(true));
  EXPECT_TRUE(auditor_->AddSecurityTest(security_test_));
  EXPECT_EQ(1, auditor_->checks().size());
}

TEST_F(SelectiveAuditorTest, SelectiveAuditorAddsSecurityCheckOk) {
  std::unique_ptr<testing::MockSecurityCheck> mock_check(
      new testing::MockSecurityCheck());
  EXPECT_CALL(*mock_check, SetRequestMetaCallback(_));
  EXPECT_CALL(*mock_check, SetGetRequestMetaCallback(_));
  auditor_->AddSecurityCheck(mock_check.release());
  EXPECT_EQ(1, auditor_->checks().size());
}

TEST_F(SelectiveAuditorTest, SelectiveAuditorFails) {
  EXPECT_CALL(mock_factory_, GetMatcher(_)).WillOnce(ReturnNull());
  EXPECT_FALSE(auditor_->AddSecurityTest(security_test_));
  EXPECT_EQ(0, auditor_->checks().size());
}

TEST_F(SelectiveAuditorTest, SelectiveAuditorScheduleOk) {
  mock_matcher_ = new testing::MockMatcher();
  EXPECT_CALL(mock_factory_, GetMatcher(_)).WillOnce(Return(mock_matcher_));
  EXPECT_CALL(*mock_matcher_, Prepare()).WillOnce(Return(true));
  EXPECT_TRUE(auditor_->AddSecurityTest(security_test_));
  ASSERT_EQ(1, auditor_->checks().size());

  EXPECT_CALL(*mock_http_client_, Schedule(NotNull(), NotNull()))
      .Times(auditor_->checks().size())
      .WillRepeatedly(Return(true));
  auditor_->ScheduleFirst(mock_request_.get());
  int original_runner_count = auditor_->checks().size();
  ASSERT_EQ(auditor_->runners().size(), original_runner_count);
  auditor_->FinishedCheckCb(auditor_->runners()[0].get());
  ASSERT_EQ(auditor_->runners().size(), original_runner_count - 1);
}

TEST_F(SelectiveAuditorTest, SelectiveAuditorScheduleWithoutChecksFail) {
  EXPECT_FALSE(auditor_->ScheduleFirst(mock_request_.get()));
}

TEST_F(SelectiveAuditorTest, SelectiveAuditorScheduleSecondTestOk) {
  testing::MockMatcher* mock_matcher_1 = new testing::MockMatcher();
  testing::MockMatcher* mock_matcher_2 = new testing::MockMatcher();
  EXPECT_CALL(mock_factory_, GetMatcher(_))
      .WillOnce(Return(mock_matcher_1))
      .WillOnce(Return(mock_matcher_2));

  EXPECT_CALL(*mock_matcher_1, Prepare()).WillOnce(Return(true));
  EXPECT_CALL(*mock_matcher_2, Prepare()).WillOnce(Return(true));
  EXPECT_TRUE(auditor_->AddSecurityTest(security_test_));
  EXPECT_TRUE(auditor_->AddSecurityTest(security_test_));
  ASSERT_EQ(2, auditor_->checks().size());

  EXPECT_CALL(*mock_http_client_, Schedule(NotNull(), NotNull()))
      .Times(auditor_->checks().size())
      .WillRepeatedly(Return(true));
  auditor_->ScheduleFirst(mock_request_.get());
  int original_runner_count = auditor_->checks().size();
  ASSERT_EQ(1, auditor_->runners().size());
  auditor_->FinishedCheckCb(auditor_->runners()[0].get());
  ASSERT_EQ(auditor_->runners().size(), original_runner_count - 1);
}

TEST_F(SelectiveAuditorTest, SelectiveAuditorCallsCrawler) {
  mock_matcher_ = new testing::MockMatcher();
  EXPECT_CALL(mock_factory_, GetMatcher(_)).WillOnce(Return(mock_matcher_));
  EXPECT_CALL(*mock_matcher_, Prepare()).WillOnce(Return(true));
  EXPECT_TRUE(auditor_->AddSecurityTest(security_test_));
  ASSERT_EQ(1, auditor_->checks().size());

  // Create the crawler. This sets the callback in the auditor and we check in
  // the test that it's called.
  testing::MockCrawler mock_crawler_(nullptr, /* client */
                                     auditor_.get(),
                                     nullptr, /* passive auditor */
                                     nullptr /* datastore */);
  EXPECT_CALL(*mock_http_client_, Schedule(NotNull(), NotNull()))
      .Times(auditor_->checks().size())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(mock_crawler_, Scrape(NotNull())).WillOnce(Return(true));
  auditor_->ScheduleFirst(mock_request_.get());
  int original_runner_count = auditor_->checks().size();
  ASSERT_EQ(auditor_->runners().size(), original_runner_count);
  auditor_->runners()[0]->RequestCallback(mock_request_.get());
}

}  // namespace plusfish
