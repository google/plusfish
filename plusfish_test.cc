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

#include "plusfish.h"

#include <csignal>
#include <string>

#include "opensource/deps/base/integral_types.h"
#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "proto/security_check.pb.h"
#include "request.h"
#include "testing/clock_mock.h"
#include "testing/crawler_mock.h"
#include "testing/datastore_mock.h"
#include "testing/hidden_objects_finder_mock.h"
#include "testing/http_client_mock.h"
#include "testing/not_found_detector_mock.h"
#include "testing/selective_auditor_mock.h"

using testing::Return;
using testing::StrEq;

namespace plusfish {

class PlusfishTest : public ::testing::Test {
 protected:
  PlusfishTest() : http_client_() {}

  void SetUp() override {
    crawler_.reset(new testing::MockCrawler(&http_client_, &selective_auditor_,
                                            nullptr /* passive auditor */,
                                            &datastore_));
    plusfish_.reset(new Plusfish(
        &clock_, crawler_.get(), &selective_auditor_, &detector_,
        &objects_finder_, 1 /* auditor runners */, &http_client_, &datastore_));
    request_.reset(new Request("http://example.org"));
  }

  SecurityCheckConfig config_;
  std::unique_ptr<Plusfish> plusfish_;
  std::unique_ptr<Request> request_;
  std::unique_ptr<testing::MockCrawler> crawler_;
  testing::MockClock clock_;
  testing::MockDataStore datastore_;
  testing::MockHiddenObjectsFinder objects_finder_;
  testing::MockHttpClient http_client_;
  testing::MockNotFoundDetector detector_;
  testing::MockSelectiveAuditor selective_auditor_;
};

TEST_F(PlusfishTest, AddURLOk) {
  std::string url("http://foo.com");
  EXPECT_CALL(datastore_, AddHost(StrEq("foo.com")));
  EXPECT_TRUE(plusfish_->AddURL(url));
}

TEST_F(PlusfishTest, AddURLFail) {
  std::string url("77*&*hpfoo.com");
  EXPECT_FALSE(plusfish_->AddURL(url));
}

TEST_F(PlusfishTest, ReportWithoutReporters) {
  EXPECT_FALSE(plusfish_->Report(config_));
}

TEST_F(PlusfishTest, InitReportingFail) {
  EXPECT_FALSE(plusfish_->InitReporting("nonexistent"));
}

TEST_F(PlusfishTest, InitReportingOK) {
  EXPECT_TRUE(plusfish_->InitReporting("TEXT"));
}

TEST_F(PlusfishTest, RunOK) {
  EXPECT_CALL(http_client_, StartNewRequests()).WillOnce(Return(true));
  EXPECT_CALL(http_client_, Poll()).WillOnce(Return(0));
  EXPECT_CALL(datastore_, audit_queue_size())
      .Times(2)
      .WillRepeatedly(Return(0));
  EXPECT_CALL(selective_auditor_, runner_count()).WillOnce(Return(0));
  EXPECT_CALL(http_client_, schedule_queue_size())
      .Times(2)
      .WillRepeatedly(Return(0));
  EXPECT_CALL(clock_, EpochTimeInMilliseconds())
      .WillOnce(Return(1557124250000))
      .WillOnce(Return(1557124251000));
  plusfish_->Run();
}

TEST_F(PlusfishTest, RunTooFastAndDelayed) {
  EXPECT_CALL(http_client_, StartNewRequests()).WillOnce(Return(true));
  EXPECT_CALL(http_client_, Poll()).WillOnce(Return(0));
  EXPECT_CALL(datastore_, audit_queue_size())
      .Times(2)
      .WillRepeatedly(Return(0));
  EXPECT_CALL(http_client_, schedule_queue_size())
      .Times(2)
      .WillRepeatedly(Return(0));
  EXPECT_CALL(clock_, EpochTimeInMilliseconds())
      .WillOnce(Return(1557124250000))
      .WillOnce(Return(1557124250001));

  // kFastLoopDelayMs = 2 (defined in plusfish.cc)
  // kFastLoopDelayMs - (1557124250001-1557124250000) = 199
  EXPECT_CALL(clock_, SleepMilliseconds(1));
  plusfish_->Run();
}

TEST_F(PlusfishTest, RunWithAuditOK) {
  EXPECT_CALL(http_client_, StartNewRequests()).WillOnce(Return(true));
  EXPECT_CALL(http_client_, Poll()).WillOnce(Return(0));
  EXPECT_CALL(datastore_, audit_queue_size())
      .Times(2)
      .WillRepeatedly(Return(1));
  EXPECT_CALL(selective_auditor_, runner_count())
      .Times(2)
      .WillRepeatedly(Return(0));
  EXPECT_CALL(http_client_, schedule_queue_size())
      .Times(2)
      .WillRepeatedly(Return(0));
  EXPECT_CALL(datastore_, GetRequestFromAuditQueue())
      .WillOnce(Return(request_.get()));
  EXPECT_CALL(selective_auditor_, ScheduleFirst(request_.get()))
      .WillOnce(Return(true));
  plusfish_->Run();
}

TEST_F(PlusfishTest, KeepRunningOrShutdownOk) {
  EXPECT_TRUE(plusfish_->KeepRunningOrShutdown());
}

TEST_F(PlusfishTest, KeepRunningOrShutdownStartsGracefulShutdown) {
  EXPECT_CALL(http_client_, enabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(http_client_, Disable());
  plusfish_->SetShutdownTime(1);
  EXPECT_TRUE(plusfish_->enabled());
  EXPECT_TRUE(plusfish_->KeepRunningOrShutdown());
}

TEST_F(PlusfishTest, KeepRunningOrShutdownCallsShutdown) {
  EXPECT_CALL(http_client_, enabled()).WillRepeatedly(Return(false));
  plusfish_->SetShutdownTime(1);
  EXPECT_FALSE(plusfish_->KeepRunningOrShutdown());
  EXPECT_FALSE(plusfish_->enabled());
}

TEST_F(PlusfishTest, ShutdownGracefulOk) {
  EXPECT_CALL(http_client_, enabled()).WillOnce(Return(true));
  EXPECT_CALL(http_client_, Disable());
  EXPECT_EQ(kint64max, plusfish_->shutdown_time());
  plusfish_->ShutdownGraceful();
  EXPECT_TRUE(plusfish_->enabled());
  EXPECT_LT(plusfish_->shutdown_time(), kint64max);
}

TEST_F(PlusfishTest, ShutdownDisablesPlusfish) {
  EXPECT_CALL(http_client_, enabled()).WillOnce(Return(true));
  EXPECT_CALL(http_client_, Disable());
  plusfish_->Shutdown();
  EXPECT_FALSE(plusfish_->enabled());
  EXPECT_FALSE(plusfish_->KeepRunningOrShutdown());
}

TEST_F(PlusfishTest, SigintShutdownGracefulOk) {
  EXPECT_CALL(http_client_, enabled()).Times(2).WillRepeatedly(Return(true));
  EXPECT_CALL(http_client_, Disable());
  EXPECT_EQ(kint64max, plusfish_->shutdown_time());
  plusfish_->SignalHandler(SIGINT);
  EXPECT_TRUE(plusfish_->enabled());
  EXPECT_LT(plusfish_->shutdown_time(), kint64max);
}

TEST_F(PlusfishTest, SigintTwiceDisables) {
  EXPECT_CALL(http_client_, enabled())
      .WillOnce(Return(false))
      .WillOnce(Return(false))
      .WillOnce(Return(true))
      .WillOnce(Return(true));
  EXPECT_CALL(http_client_, Disable());
  plusfish_->SignalHandler(SIGINT);
  plusfish_->SignalHandler(SIGINT);
  EXPECT_FALSE(plusfish_->enabled());
}

TEST_F(PlusfishTest, SigTermCausesShutdown) {
  EXPECT_CALL(http_client_, enabled()).WillOnce(Return(true));
  EXPECT_CALL(http_client_, Disable());
  plusfish_->SignalHandler(SIGTERM);
  EXPECT_FALSE(plusfish_->enabled());
}

}  // namespace plusfish
