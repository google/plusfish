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

#include "audit/generic_security_check.h"

#include <memory>

#include "gtest/gtest.h"
#include "proto/security_check.pb.h"
#include "request.h"
#include "testing/generator_mock.h"
#include "testing/response_matcher_mock.h"

using testing::Return;

namespace plusfish {

class GenericSecurityCheckTest : public ::testing::Test {
 protected:
  GenericSecurityCheckTest() {}

  void SetUp() override {
    test_.set_name("test name");
    generator_ = new testing::MockGenerator();
    matcher_ = new testing::MockResponseMatcher();
    check_.reset(new GenericSecurityCheck(generator_, matcher_, test_));
  }

  testing::MockGenerator* generator_;
  testing::MockResponseMatcher* matcher_;
  std::unique_ptr<GenericSecurityCheck> check_;
  SecurityTest test_;
};

TEST_F(GenericSecurityCheckTest, CreateRequestsGeneratorFail) {
  std::vector<std::unique_ptr<Request>> requests;
  Request test_request("http://test");
  EXPECT_CALL(*generator_, Generate(&test_request, &requests))
      .WillOnce(Return(false));
  EXPECT_FALSE(check_->CreateRequests(test_request, &requests));
}

TEST_F(GenericSecurityCheckTest, CreateRequestsGeneratorOK) {
  std::vector<std::unique_ptr<Request>> requests;
  Request test_request("http://test");
  EXPECT_CALL(*generator_, Generate(&test_request, &requests))
      .WillOnce(Return(true));
  EXPECT_TRUE(check_->CreateRequests(test_request, &requests));
}

TEST_F(GenericSecurityCheckTest, EvaluateSingle) {
  Request test_request("http://test");
  EXPECT_CALL(*matcher_, MatchSingle(&test_request)).WillOnce(Return(true));
  EXPECT_TRUE(check_->EvaluateSingle(&test_request));
}

TEST_F(GenericSecurityCheckTest, Evaluate) {
  std::vector<std::unique_ptr<Request>> requests;
  EXPECT_CALL(*matcher_, Match(&requests)).WillOnce(Return(true));
  EXPECT_TRUE(check_->Evaluate(requests));
}

}  // namespace plusfish
