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

#include "audit/security_check_runner.h"

#include <functional>
#include <memory>
#include <vector>

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "testing/http_client_mock.h"
#include "testing/request_mock.h"
#include "testing/security_check_mock.h"
#include "testing/selective_auditor_mock.h"

using std::placeholders::_1;
using testing::_;
using testing::DoAll;
using testing::NotNull;
using testing::Return;
using testing::ReturnRef;

namespace plusfish {

// This is a workaround that is needed due to SetPointeeArg not supporting the
// copying of std::unique std::vectors. This is based on a similar snippet in
// contentads/xfa/verification/util/mock-dremel-query-executor.h
ACTION_TEMPLATE(SetUniquePtrVector, HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(copy_from)) {
  // Move the requests into the tested std::vector.
  for (auto& req : *copy_from) {
    ::testing::get<k>(args)->push_back(std::move(req));
  }
}

class SecurityCheckRunnerTest : public ::testing::Test {
 public:
  SecurityCheckRunnerTest() : check_name_("name"), url_("http://example.org") {}

  void SetUp() override {
    request_.reset(new testing::MockRequest(url_));
    check_.reset(new testing::MockSecurityCheck());
    http_client_.reset(new testing::MockHttpClient());
    ON_CALL(*check_, name()).WillByDefault(ReturnRef(check_name_));
    ON_CALL(*check_, severity()).WillByDefault(Return(MODERATE));
    ON_CALL(*request_, url()).WillByDefault(ReturnRef(url_));
  }

  std::string check_name_;
  std::string url_;
  std::unique_ptr<testing::MockHttpClient> http_client_;
  std::unique_ptr<testing::MockSecurityCheck> check_;
  std::unique_ptr<testing::MockRequest> request_;
};

TEST_F(SecurityCheckRunnerTest, CreatesRequestsFailsOnGenerator) {
  SecurityCheckRunner runner_(check_.get(), request_.get());
  EXPECT_CALL(*check_, CreateRequests(_, NotNull())).WillOnce(Return(false));
  ASSERT_FALSE(runner_.Run(http_client_.get()));
}

TEST_F(SecurityCheckRunnerTest, CreatesRequests) {
  std::vector<std::unique_ptr<Request>> requests;
  requests.push_back(
      std::unique_ptr<testing::MockRequest>(new testing::MockRequest(url_)));

  SecurityCheckRunner runner_(check_.get(), request_.get());
  EXPECT_CALL(*check_, CreateRequests(_, NotNull()))
      .WillOnce(DoAll(SetUniquePtrVector<1>(&requests), Return(true)));
  EXPECT_CALL(*http_client_, Schedule(NotNull(), NotNull()))
      .WillOnce(Return(true));
  ASSERT_TRUE(runner_.Run(http_client_.get()));
}

TEST_F(SecurityCheckRunnerTest, CreatesRequestsFailOnSchedule) {
  std::vector<std::unique_ptr<Request>> requests;
  requests.push_back(
      std::unique_ptr<testing::MockRequest>(new testing::MockRequest(url_)));

  SecurityCheckRunner runner_(check_.get(), request_.get());
  EXPECT_CALL(*check_, CreateRequests(_, NotNull()))
      .WillOnce(DoAll(SetUniquePtrVector<1>(&requests), Return(true)));
  EXPECT_CALL(*http_client_, Schedule(NotNull(), NotNull()))
      .WillOnce(Return(false));
  ASSERT_FALSE(runner_.Run(http_client_.get()));
}

TEST_F(SecurityCheckRunnerTest, CallbackEvaluateSingle) {
  // First set the requests std::vector.
  std::vector<std::unique_ptr<Request>> requests;
  requests.push_back(
      std::unique_ptr<testing::MockRequest>(new testing::MockRequest(url_)));

  SecurityCheckRunner runner_(check_.get(), request_.get());
  EXPECT_CALL(*check_, CreateRequests(_, NotNull()))
      .WillOnce(DoAll(SetUniquePtrVector<1>(&requests), Return(true)));
  EXPECT_CALL(*check_, CanEvaluateInSerial()).WillOnce(Return(true));
  EXPECT_CALL(*check_, EvaluateSingle(request_.get())).WillOnce(Return(false));
  EXPECT_CALL(*http_client_, Schedule(NotNull(), NotNull()))
      .WillOnce(Return(true));
  ASSERT_TRUE(runner_.Run(http_client_.get()));
  // Now evaluate and ensure it's finished.
  EXPECT_CALL(*check_, CanEvaluateInSerial()).WillOnce(Return(true));
  EXPECT_CALL(*check_, EvaluateSingle(NotNull())).WillOnce(Return(true));
  runner_.RequestCallback(request_.get());
  EXPECT_TRUE(runner_.finished());
}

TEST_F(SecurityCheckRunnerTest, OriginalMatchesPositiveAndIsSkipped) {
  // First set the requests std::vector.
  std::vector<std::unique_ptr<Request>> requests;
  requests.push_back(
      std::unique_ptr<testing::MockRequest>(new testing::MockRequest(url_)));

  SecurityCheckRunner runner_(check_.get(), request_.get());
  EXPECT_CALL(*check_, CreateRequests(_, NotNull()))
      .WillOnce(DoAll(SetUniquePtrVector<1>(&requests), Return(true)));
  EXPECT_CALL(*check_, CanEvaluateInSerial()).WillOnce(Return(true));
  EXPECT_CALL(*check_, EvaluateSingle(NotNull())).WillOnce(Return(true));
  ASSERT_FALSE(runner_.Run(http_client_.get()));
}

TEST_F(SecurityCheckRunnerTest, CleanupCallbackCalledOnFinish) {
  testing::MockSelectiveAuditor auditor;
  EXPECT_CALL(auditor, CleanupCallback(NotNull()));
  // First set the requests std::vector.
  std::vector<std::unique_ptr<Request>> requests;
  requests.push_back(
      std::unique_ptr<testing::MockRequest>(new testing::MockRequest(url_)));

  SecurityCheckRunner runner_(check_.get(), request_.get());
  runner_.OnCheckDone(
      std::bind(&testing::MockSelectiveAuditor::CleanupCallback, &auditor, _1));
  EXPECT_CALL(*check_, CreateRequests(_, NotNull()))
      .WillOnce(DoAll(SetUniquePtrVector<1>(&requests), Return(true)));
  EXPECT_CALL(*http_client_, Schedule(NotNull(), NotNull()))
      .WillOnce(Return(true));
  ASSERT_TRUE(runner_.Run(http_client_.get()));
  // Now evaluate and ensure it's finished.
  EXPECT_CALL(*check_, CanEvaluateInSerial()).WillOnce(Return(true));
  EXPECT_CALL(*check_, EvaluateSingle(NotNull())).WillOnce(Return(true));
  runner_.RequestCallback(request_.get());
  EXPECT_TRUE(runner_.finished());
}

TEST_F(SecurityCheckRunnerTest, CallbackEvaluateNotCalledWhenNotFinished) {
  testing::MockSelectiveAuditor auditor;
  EXPECT_CALL(*check_, CanEvaluateInSerial()).WillOnce(Return(false));
  SecurityCheckRunner runner_(check_.get(), request_.get());
  // Set the callback without having expectations for this method on the mock.
  // So when the method would be called, this test will fail since the cleanup
  // callback should only be called for 'finished' runners.
  runner_.OnCheckDone(
      std::bind(&testing::MockSelectiveAuditor::CleanupCallback, &auditor, _1));
  runner_.RequestCallback(request_.get());
  EXPECT_FALSE(runner_.finished());
}

}  // namespace plusfish
