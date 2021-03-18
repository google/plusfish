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

#include "audit/matchers/timing_matcher.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "proto/http_request.pb.h"
#include "proto/matching.pb.h"
#include "request.h"
#include "testing/datastore_mock.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SetArgPointee;

namespace plusfish {

class TimingMatcherTest : public ::testing::Test {
 protected:
  TimingMatcherTest() : request_("http://example.org") {
    meta_callback_ = std::bind(&testing::MockDataStore::GetRequestMetadata,
                               &datastore_, std::placeholders::_1,
                               std::placeholders::_2, std::placeholders::_3);
  }
  MatchRule_Match match_;
  Request request_;
  testing::MockDataStore datastore_;
  std::function<bool(const int64 request_id, const MetaData_Type type,
                     int64* value)>
      meta_callback_;
};

TEST_F(TimingMatcherTest, MatchAnyOk) {
  match_.mutable_timing()->set_min_duration_ms(2000);
  match_.mutable_timing()->set_max_duration_ms(3000);
  TimingMatcher matcher(match_, meta_callback_);
  EXPECT_CALL(
      datastore_,
      GetRequestMetadata(_, MetaData::AVERAGE_APPLICATION_TIME_USEC, NotNull()))
      .WillOnce(DoAll(SetArgPointee<2>(0), Return(true)));

  request_.set_client_time_application_usec(2500000);
  ASSERT_TRUE(matcher.Prepare());
  EXPECT_TRUE(matcher.MatchAny(&request_, nullptr));
}

TEST_F(TimingMatcherTest, MatchHighAverageResponseTimeOk) {
  match_.mutable_timing()->set_min_duration_ms(2000);
  match_.mutable_timing()->set_max_duration_ms(3000);
  TimingMatcher matcher(match_, meta_callback_);
  EXPECT_CALL(
      datastore_,
      GetRequestMetadata(_, MetaData::AVERAGE_APPLICATION_TIME_USEC, NotNull()))
      .WillOnce(DoAll(SetArgPointee<2>(2000000), Return(true)));

  request_.set_client_time_application_usec(4500000);
  ASSERT_TRUE(matcher.Prepare());
  EXPECT_TRUE(matcher.MatchAny(&request_, nullptr));
}

TEST_F(TimingMatcherTest, AverageResponseTimeDoesNotYieldFalsePositive) {
  // The normal response time is so slow that it falls right between the
  // tested duration. Since the expected response times are relative to average
  // response time: there should not be a match.
  int64 normal_response_time = 250000;
  match_.mutable_timing()->set_min_duration_ms(2000);
  match_.mutable_timing()->set_max_duration_ms(3000);
  TimingMatcher matcher(match_, meta_callback_);
  EXPECT_CALL(
      datastore_,
      GetRequestMetadata(_, MetaData::AVERAGE_APPLICATION_TIME_USEC, NotNull()))
      .WillOnce(DoAll(SetArgPointee<2>(normal_response_time), Return(true)));

  request_.set_client_time_application_usec(normal_response_time);
  ASSERT_TRUE(matcher.Prepare());
  EXPECT_FALSE(matcher.MatchAny(&request_, nullptr));
}

TEST_F(TimingMatcherTest, DoesNotMatchTooSlowRequest) {
  match_.mutable_timing()->set_min_duration_ms(2000);
  match_.mutable_timing()->set_max_duration_ms(3000);
  TimingMatcher matcher(match_, meta_callback_);
  EXPECT_CALL(
      datastore_,
      GetRequestMetadata(_, MetaData::AVERAGE_APPLICATION_TIME_USEC, NotNull()))
      .WillOnce(DoAll(SetArgPointee<2>(0), Return(true)));

  request_.set_client_time_application_usec(4000000);
  ASSERT_TRUE(matcher.Prepare());
  EXPECT_FALSE(matcher.MatchAny(&request_, nullptr));
}

TEST_F(TimingMatcherTest, DoesNotMatchTooFastRequest) {
  match_.mutable_timing()->set_min_duration_ms(2000);
  match_.mutable_timing()->set_max_duration_ms(3000);
  TimingMatcher matcher(match_, meta_callback_);
  EXPECT_CALL(
      datastore_,
      GetRequestMetadata(_, MetaData::AVERAGE_APPLICATION_TIME_USEC, NotNull()))
      .WillOnce(DoAll(SetArgPointee<2>(0), Return(true)));

  request_.set_client_time_application_usec(1);
  ASSERT_TRUE(matcher.Prepare());
  EXPECT_FALSE(matcher.MatchAny(&request_, nullptr));
}

TEST_F(TimingMatcherTest, DoesPrepareWhenMaxDurationIsTooLow) {
  match_.mutable_timing()->set_min_duration_ms(3000);
  match_.mutable_timing()->set_max_duration_ms(2000);
  TimingMatcher matcher(match_, meta_callback_);
  EXPECT_FALSE(matcher.Prepare());
}

}  // namespace plusfish
