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

#include "not_found_detector.h"

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/node_hash_set.h"
#include "request.h"
#include "response.h"
#include "testing/http_client_mock.h"
#include "testing/request_mock.h"
#include "util/html_fingerprint.h"

using testing::ContainsRegex;
using testing::Return;
using testing::ReturnNull;

namespace plusfish {

class NotFoundDetectorTest : public ::testing::Test {
 public:
  NotFoundDetectorTest() : target_url_("http://test/foo.html") {}

  void SetUp() override {
    detector_.reset(new NotFoundDetector());

    fake_schedule_callback_count_ = 0;
    fake_schedule_callback_return_value_ = true;
    fake_store_fp_callback_count_ = 0;
    fake_schedule_callback_req_urls_.clear();

    // Set the two callbacks.
    detector_->SetHttpClientScheduleCallback([this](const Request* req) {
      this->fake_schedule_callback_req_urls_.insert(req->url());
      this->fake_schedule_callback_count_++;
      return fake_schedule_callback_return_value_;
    });

    detector_->SetDatastoreFingerprintCallback(
        [this](const HtmlFingerprint* fp) {
          this->fake_store_fp_callback_count_++;
        });
  }

  int fake_store_fp_callback_count_;
  int fake_schedule_callback_count_;
  bool fake_schedule_callback_return_value_;
  absl::node_hash_set<std::string> fake_schedule_callback_req_urls_;
  std::string target_url_;
  std::unique_ptr<NotFoundDetector> detector_;
};

TEST_F(NotFoundDetectorTest, AddUrlScheduledCorrectRequests) {
  detector_->AddUrl(target_url_);
  EXPECT_EQ(9, fake_schedule_callback_count_);
  for (const auto& url : fake_schedule_callback_req_urls_) {
    EXPECT_THAT(url, ContainsRegex("http:\\/\\/test:80\\/[0-9]+\\.[a-z]+"));
  }
}

TEST_F(NotFoundDetectorTest, AddUrlScheduledCorrectRequestsAndAddsSlash) {
  detector_->AddUrl("http://test:80");
  EXPECT_EQ(9, fake_schedule_callback_count_);
  for (const auto& url : fake_schedule_callback_req_urls_) {
    EXPECT_THAT(url, ContainsRegex("http:\\/\\/test:80\\/[0-9]+\\.[a-z]+"));
  }
}

TEST_F(NotFoundDetectorTest, AddUrlScheduleFailsCorrect) {
  fake_schedule_callback_return_value_ = false;
  detector_->AddUrl(target_url_);
  EXPECT_EQ(0, detector_->probed_urls().size());
}

TEST_F(NotFoundDetectorTest, RequestCallbackNoResponse) {
  testing::MockRequest request("http://test:80/foo.html");
  EXPECT_CALL(request, response()).WillOnce(ReturnNull());
  EXPECT_EQ(0, detector_->RequestCallback(&request));
  EXPECT_EQ(0, fake_store_fp_callback_count_);
}

TEST_F(NotFoundDetectorTest, RequestCallbackOk) {
  testing::MockRequest request("http://test:80/foo.html");
  Response response;
  response.Parse(
      "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
      "<html><p>a la la</p></html>");

  EXPECT_CALL(request, response()).Times(2).WillRepeatedly(Return(&response));
  EXPECT_EQ(1, detector_->RequestCallback(&request));
  EXPECT_EQ(1, fake_store_fp_callback_count_);
}

}  // namespace plusfish
