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

#include "audit/response_time_check.h"

#include <memory>

#include "gtest/gtest.h"
#include "proto/http_request.pb.h"
#include "request.h"
#include "testing/datastore_mock.h"

using testing::Return;

namespace plusfish {

class ResponseTimeCheckTest : public ::testing::Test {
 protected:
  ResponseTimeCheckTest() : number_requests_(4), threshold_ms_(800) {}

  void SetUp() override {
    response_time_check_.reset(
        new ResponseTimeCheck(number_requests_, threshold_ms_));
    response_time_check_->SetRequestMetaCallback(std::bind(
        &testing::MockDataStore::AddRequestMetadata, &datastore_,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  }

  int number_requests_;
  int threshold_ms_;
  testing::MockDataStore datastore_;
  std::unique_ptr<ResponseTimeCheck> response_time_check_;
  std::vector<std::unique_ptr<Request>> requests_;
};

TEST_F(ResponseTimeCheckTest, CreatesRequestsOk) {
  Request request("http://example.org");
  EXPECT_TRUE(response_time_check_->CreateRequests(request, &requests_));
  EXPECT_EQ(number_requests_, requests_.size());
}

TEST_F(ResponseTimeCheckTest, EvaluatesBelowThreshold) {
  Request request("http://example.org");
  int64 request_id = 42;
  int64 average_response_time = 84;
  request.set_id(request_id);
  EXPECT_TRUE(response_time_check_->CreateRequests(request, &requests_));
  // Set a response time to compare.
  for (auto& test_request : requests_) {
    test_request->set_client_time_application_usec(average_response_time);
  }
  EXPECT_CALL(
      datastore_,
      AddRequestMetadata(request_id, MetaData::AVERAGE_APPLICATION_TIME_USEC,
                         average_response_time))
      .WillOnce(Return(true));
  EXPECT_FALSE(response_time_check_->Evaluate(requests_));
}

TEST_F(ResponseTimeCheckTest, EvaluatesExceedsThreshold) {
  Request request("http://example.org");
  int64 request_id = 42;
  int64 average_response_time_usec = 801000;
  request.set_id(request_id);
  EXPECT_TRUE(response_time_check_->CreateRequests(request, &requests_));
  // Set a response time to compare.
  for (auto& test_request : requests_) {
    test_request->set_client_time_application_usec(average_response_time_usec);
  }
  EXPECT_CALL(
      datastore_,
      AddRequestMetadata(request_id, MetaData::AVERAGE_APPLICATION_TIME_USEC,
                         average_response_time_usec))
      .WillOnce(Return(true));
  EXPECT_TRUE(response_time_check_->Evaluate(requests_));
}

TEST_F(ResponseTimeCheckTest, EvaluatesFailsOnSettingResponseTime) {
  Request request("http://example.org");
  int64 request_id = 42;
  int64 average_response_time_usec = 84;
  request.set_id(request_id);
  EXPECT_TRUE(response_time_check_->CreateRequests(request, &requests_));
  for (auto& test_request : requests_) {
    test_request->set_client_time_application_usec(average_response_time_usec);
  }
  EXPECT_CALL(
      datastore_,
      AddRequestMetadata(request_id, MetaData::AVERAGE_APPLICATION_TIME_USEC,
                         average_response_time_usec))
      .WillOnce(Return(false));
  EXPECT_FALSE(response_time_check_->Evaluate(requests_));
}

}  // namespace plusfish
