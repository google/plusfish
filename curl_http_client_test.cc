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

#include "curl_http_client.h"

#include <curl/curl.h>
#include <gtest/gtest.h>

#include <string>

#include "absl/flags/flag.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "request.h"
#include "testing/curl_mock.h"
#include "testing/ratelimiter_mock.h"
#include "testing/request_mock.h"
#include "util/curl_util.h"
#include "util/http_util.h"

ABSL_DECLARE_FLAG(std::string, user_agent);
ABSL_DECLARE_FLAG(std::string, http_proxy);
ABSL_DECLARE_FLAG(std::string, client_ssl_cert);
ABSL_DECLARE_FLAG(std::string, client_ssl_key);
ABSL_DECLARE_FLAG(std::string, client_ssl_key_password);
ABSL_DECLARE_FLAG(int32_t, max_response_read_size);
ABSL_DECLARE_FLAG(int32_t, connection_limit);

using testing::_;
using testing::AtLeast;
using testing::DoAll;
using testing::HasSubstr;
using testing::NotNull;
using testing::Return;
using testing::ReturnNull;
using testing::StrEq;

ACTION_P(SetArg2ToDouble, value) { *static_cast<double*>(arg2) = value; }

// Match against calculated timeout (timeval struct).
MATCHER_P(MatchTimeval, value, "Compare the timeval struct") {
  return arg->tv_usec == value.tv_usec && arg->tv_sec == value.tv_sec;
}

namespace plusfish {

class CurlHttpClientTest : public ::testing::Test {
 public:
  CurlHttpClientTest()
      : mock_curl_(new testing::MockCurl()), curlm_(nullptr), curl_(nullptr) {
    curlm_ = curl_multi_init();
    curl_ = curl_easy_init();
  }

  ~CurlHttpClientTest() override {}

  void SetUp() override {
    mock_rate_limiter_ = new testing::MockRateLimiter();
    ON_CALL(*mock_rate_limiter_, max_rate()).WillByDefault(Return(1));

    curl_client_.reset(new CurlHttpClient(mock_curl_, mock_rate_limiter_));
    handle_data_.reset(new CurlHandleData());
    handle_data_->easy_handle = curl_;
    handle_data_->request = nullptr;
    handle_data_->header_list = nullptr;

    ON_CALL(*mock_curl_, MultiInit()).WillByDefault(Return(curlm_));
    ON_CALL(*mock_curl_,
            MultiSetOpt(NotNull(), CURLMOPT_MAXCONNECTS, NotNull()))
        .WillByDefault(Return(CURLM_OK));
    ON_CALL(*mock_curl_, MultiCleanup(curlm_)).WillByDefault(Return(CURLM_OK));
    ON_CALL(*mock_curl_, AppendSlist(_, _)).WillByDefault(Return(true));
  }

  void TearDown() override { curl_multi_cleanup(curlm_); }

  void SetNewHandleExpectations() {
    EXPECT_CALL(*mock_curl_, EasyInit()).WillOnce(Return(curl_));
    EXPECT_CALL(*mock_curl_,
                EasySetWriteCallback(curl_, CURLOPT_WRITEFUNCTION, _))
        .WillOnce(Return(CURLE_OK));

    EXPECT_CALL(*mock_curl_, EasySetOptInt64(_, _, _))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(CURLE_OK));

    EXPECT_CALL(*mock_curl_, EasySetOpt(_, _, _))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(CURLE_OK));
  }

  void SetSetRequestHeadersExpectations() {
    EXPECT_CALL(*mock_curl_, AppendSlist(_, _))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
  }

  void SetScheduleExpectations() {
    SetNewHandleExpectations();
    SetSetRequestHeadersExpectations();
  }

  void SetInitializeExpectations() {
    EXPECT_CALL(*mock_curl_, MultiInit()).WillOnce(Return(curlm_));
    EXPECT_CALL(*mock_curl_,
                MultiSetOpt(NotNull(), CURLMOPT_MAXCONNECTS, NotNull()))
        .WillOnce(Return(CURLM_OK));
    EXPECT_CALL(*mock_curl_, MultiCleanup(curlm_)).WillOnce(Return(CURLM_OK));
  }

  void SetPollNoMessagesExpectations() {
    struct timeval expected_tv;
    expected_tv.tv_sec = 1;
    expected_tv.tv_usec = 0;
    EXPECT_CALL(*mock_curl_, GetTimeout(curlm_)).WillOnce(Return(1000));
    EXPECT_CALL(*mock_curl_, Select(curlm_, MatchTimeval(expected_tv)))
        .WillOnce(Return(0));
    EXPECT_CALL(*mock_curl_, MultiPerform(curlm_, _))
        .WillOnce(Return(CURLM_OK));
    EXPECT_CALL(*mock_curl_, MultiInfoRead(curlm_, _)).WillOnce(ReturnNull());
  }

 protected:
  testing::MockCurl* mock_curl_;
  testing::MockRateLimiter* mock_rate_limiter_;
  CURLM* curlm_;
  CURL* curl_;
  std::unique_ptr<CurlHttpClient> curl_client_;
  std::unique_ptr<CurlHandleData> handle_data_;
};

TEST_F(CurlHttpClientTest, CurlCallbackOk) {
  std::string new_content("this will be added");
  absl::SetFlag(&FLAGS_max_response_read_size, new_content.length());

  testing::MockRequest mock_request("http://foo");

  EXPECT_EQ(new_content.length(),
            plusfish::CurlCallback(new_content.c_str(), new_content.length(), 1,
                                   &mock_request));
  EXPECT_EQ(new_content, mock_request.raw_response());
  curl_easy_cleanup(curl_);
}

TEST_F(CurlHttpClientTest, CurlCallbackExceedsLimit) {
  std::string content("12345");
  std::string new_content("6789");
  absl::SetFlag(&FLAGS_max_response_read_size, content.length());

  testing::MockRequest mock_request("http://foo");

  // Set a response std::string with the maximum allowed length.
  EXPECT_EQ(content.length(),
            plusfish::CurlCallback(content.c_str(), content.length(), 1,
                                   &mock_request));
  // The extra data given via the callback will not be added. This because the
  // existing response size is the maximum allowed size already.
  EXPECT_EQ(0, plusfish::CurlCallback(new_content.c_str(), new_content.length(),
                                      1, &mock_request));
  EXPECT_EQ(content, mock_request.raw_response());
  curl_easy_cleanup(curl_);
}

TEST_F(CurlHttpClientTest, CurlCallbackPartiallyExceedsLimit) {
  std::string initial_content("123");
  std::string new_content("456");
  // Allow one additional byte to be read.
  absl::SetFlag(&FLAGS_max_response_read_size, 1 + initial_content.length());

  testing::MockRequest mock_request("http://foo");

  EXPECT_EQ(initial_content.length(),
            plusfish::CurlCallback(initial_content.c_str(),
                                   initial_content.length(), 1, &mock_request));
  // Try fitting in the additional content fails.
  EXPECT_EQ(0, plusfish::CurlCallback(new_content.c_str(), new_content.length(),
                                      1, &mock_request));
  EXPECT_EQ(initial_content, mock_request.raw_response());
  curl_easy_cleanup(curl_);
}

TEST_F(CurlHttpClientTest, InitializeOK) {
  SetInitializeExpectations();
  EXPECT_TRUE(curl_client_->Initialize());
  curl_easy_cleanup(curl_);
}

TEST_F(CurlHttpClientTest, InitializeSetoptFail) {
  EXPECT_CALL(*mock_curl_, MultiInit()).WillOnce(Return(curlm_));
  EXPECT_CALL(*mock_curl_,
              MultiSetOpt(NotNull(), CURLMOPT_MAXCONNECTS, NotNull()))
      .WillOnce(Return(CURLM_INTERNAL_ERROR));
  EXPECT_FALSE(curl_client_->Initialize());
  curl_easy_cleanup(curl_);
}

TEST_F(CurlHttpClientTest, ScheduleOK) {
  Request request_("http://example.org");
  EXPECT_TRUE(curl_client_->Initialize());
  EXPECT_TRUE(curl_client_->Schedule(&request_));
  EXPECT_EQ(1, curl_client_->schedule_queue_size());
  curl_easy_cleanup(curl_);
}

TEST_F(CurlHttpClientTest, PollNoMessagesOK) {
  SetInitializeExpectations();
  EXPECT_TRUE(curl_client_->Initialize());

  SetScheduleExpectations();
  Request request_("http://example.org");
  EXPECT_TRUE(curl_client_->Schedule(&request_));
  ASSERT_EQ(1, curl_client_->schedule_queue_size());

  struct timeval expected_tv;
  expected_tv.tv_sec = 1;
  expected_tv.tv_usec = 0;
  EXPECT_CALL(*mock_curl_, GetTimeout(curlm_)).WillOnce(Return(1000));
  EXPECT_CALL(*mock_curl_, Select(curlm_, MatchTimeval(expected_tv)))
      .WillOnce(Return(0));

  EXPECT_CALL(*mock_rate_limiter_, TakeRateSlot()).WillOnce(Return(true));
  EXPECT_EQ(1, curl_client_->StartNewRequests());
  EXPECT_EQ(0, curl_client_->Poll());
}

TEST_F(CurlHttpClientTest, PollOneMessageNotDone) {
  CURLMsg curl_msg;
  curl_msg.msg = CURLMSG_NONE;
  SetInitializeExpectations();
  EXPECT_TRUE(curl_client_->Initialize());

  SetScheduleExpectations();
  Request request_("http://example.org");
  EXPECT_TRUE(curl_client_->Schedule(&request_));
  ASSERT_EQ(1, curl_client_->schedule_queue_size());

  EXPECT_CALL(*mock_rate_limiter_, TakeRateSlot()).WillOnce(Return(true));
  EXPECT_CALL(*mock_curl_, GetTimeout(curlm_)).WillOnce(Return(1000));
  EXPECT_CALL(*mock_curl_, Select(curlm_, _)).WillOnce(Return(1));
  EXPECT_CALL(*mock_curl_, MultiAddHandle(curlm_, _))
      .WillOnce(Return(CURLM_OK));
  EXPECT_CALL(*mock_curl_, MultiPerform(curlm_, _))
      .WillRepeatedly(Return(CURLM_OK));
  EXPECT_CALL(*mock_curl_, MultiInfoRead(curlm_, _))
      .WillOnce(Return(&curl_msg))
      .WillOnce(Return(nullptr));
  EXPECT_EQ(1, curl_client_->StartNewRequests());
  curl_client_->Poll();
}

TEST_F(CurlHttpClientTest, PollOK) {
  CURLMsg curl_msg;
  curl_msg.msg = CURLMSG_DONE;
  curl_msg.easy_handle = curl_;

  SetInitializeExpectations();
  EXPECT_TRUE(curl_client_->Initialize());

  SetScheduleExpectations();
  testing::MockRequest mock_request("http://www.example.com");
  handle_data_->easy_handle = curl_;
  handle_data_->request = &mock_request;
  EXPECT_TRUE(curl_client_->Schedule(&mock_request));
  ASSERT_EQ(1, curl_client_->schedule_queue_size());

  EXPECT_CALL(*mock_rate_limiter_, TakeRateSlot()).WillOnce(Return(true));
  EXPECT_CALL(*mock_curl_, GetTimeout(curlm_)).WillOnce(Return(1000));
  EXPECT_CALL(*mock_curl_, Select(curlm_, _)).WillOnce(Return(1));
  EXPECT_CALL(*mock_curl_, MultiAddHandle(curlm_, _))
      .WillOnce(Return(CURLM_OK));
  EXPECT_CALL(*mock_curl_, MultiPerform(curlm_, _))
      .WillRepeatedly(Return(CURLM_OK));
  EXPECT_CALL(*mock_curl_, MultiInfoRead(curlm_, _))
      .WillOnce(Return(&curl_msg))
      .WillOnce(Return(nullptr));

  // Several information gathering expectations.
  EXPECT_CALL(*mock_curl_,
              EasyGetInfo(NotNull(), CURLINFO_PRIMARY_IP, NotNull()))
      .WillOnce(Return(CURLE_OK));
  EXPECT_CALL(*mock_curl_,
              EasyGetInfo(NotNull(), CURLINFO_TOTAL_TIME, NotNull()))
      .WillOnce(DoAll(SetArg2ToDouble(0), Return(CURLE_OK)));
  EXPECT_CALL(*mock_curl_,
              EasyGetInfo(NotNull(), CURLINFO_CONNECT_TIME, NotNull()))
      .WillOnce(DoAll(SetArg2ToDouble(0), Return(CURLE_OK)));

  // Final callback & cleanup expectations.
  EXPECT_CALL(mock_request, DoneCb());
  EXPECT_CALL(*mock_curl_, EasyGetHandleData(NotNull()))
      .WillOnce(Return(handle_data_.get()));
  EXPECT_EQ(1, curl_client_->StartNewRequests());
  EXPECT_EQ(0, curl_client_->Poll());
}

TEST_F(CurlHttpClientTest, DisabledDoesntSchedule) {
  Request request_("http://example.org");
  EXPECT_TRUE(curl_client_->Initialize());
  EXPECT_TRUE(curl_client_->enabled());
  curl_client_->Disable();
  EXPECT_FALSE(curl_client_->enabled());
  EXPECT_FALSE(curl_client_->Schedule(&request_));
  EXPECT_EQ(0, curl_client_->schedule_queue_size());
  curl_easy_cleanup(curl_);
}

TEST_F(CurlHttpClientTest, NewHandleWithSSLOk) {
  absl::SetFlag(&FLAGS_client_ssl_cert, "path/to/cert");
  absl::SetFlag(&FLAGS_client_ssl_key, "path/to/key");
  absl::SetFlag(&FLAGS_client_ssl_key_password, "***");
  EXPECT_CALL(*mock_curl_,
              EasySetWriteCallback(curl_, CURLOPT_WRITEFUNCTION, _))
      .WillOnce(Return(CURLE_OK));
  EXPECT_CALL(*mock_curl_, EasyInit()).WillOnce(Return(curl_));
  EXPECT_CALL(*mock_curl_, EasySetOpt(_, _, _))
      .WillRepeatedly(Return(CURLE_OK));
  EXPECT_CALL(*mock_curl_, EasySetOptInt64(_, _, _))
      .WillRepeatedly(Return(CURLE_OK));
  EXPECT_CALL(*mock_curl_, EasySetOpt(_, CURLOPT_SSLCERT, _))
      .WillOnce(Return(CURLE_OK));
  EXPECT_CALL(*mock_curl_, EasySetOpt(_, CURLOPT_SSLKEY, _))
      .WillOnce(Return(CURLE_OK));
  EXPECT_CALL(*mock_curl_, EasySetOpt(_, CURLOPT_KEYPASSWD, _))
      .WillOnce(Return(CURLE_OK));

  Request request("http://example.org");

  EXPECT_TRUE(curl_client_->NewHandle(&request) != nullptr);
}

TEST_F(CurlHttpClientTest, NewHandleWithSSLFails) {
  absl::SetFlag(&FLAGS_client_ssl_cert, "path/to/cert");
  absl::SetFlag(&FLAGS_client_ssl_key, "path/to/key");
  EXPECT_CALL(*mock_curl_, EasyInit()).WillOnce(Return(curl_));
  EXPECT_CALL(*mock_curl_, EasySetOpt(_, _, _))
      .WillRepeatedly(Return(CURLE_OK));
  EXPECT_CALL(*mock_curl_, EasySetOptInt64(_, _, _))
      .WillRepeatedly(Return(CURLE_OK));
  EXPECT_CALL(*mock_curl_, EasySetOpt(_, CURLOPT_SSLCERT, _))
      .WillOnce(Return(CURLE_OK));
  EXPECT_CALL(*mock_curl_, EasySetOpt(_, CURLOPT_SSLKEY, _))
      .WillOnce(Return(CURLE_OUT_OF_MEMORY));

  Request request("http://example.org");

  EXPECT_EQ(curl_client_->NewHandle(&request), nullptr);
}

TEST_F(CurlHttpClientTest, NewHandleWithProxyOk) {
  std::string proxy = "http://127.0.0.1:8080/";
  absl::SetFlag(&FLAGS_http_proxy, proxy);
  EXPECT_CALL(*mock_curl_,
              EasySetWriteCallback(curl_, CURLOPT_WRITEFUNCTION, _))
      .WillOnce(Return(CURLE_OK));
  EXPECT_CALL(*mock_curl_, EasyInit()).WillOnce(Return(curl_));
  EXPECT_CALL(*mock_curl_, EasySetOpt(_, _, _))
      .WillRepeatedly(Return(CURLE_OK));
  EXPECT_CALL(*mock_curl_, EasySetOptInt64(_, _, _))
      .WillRepeatedly(Return(CURLE_OK));
  // 3rd argument is the proxy but a void* pointer so Pointee matching does not
  // work.
  EXPECT_CALL(*mock_curl_, EasySetOpt(_, CURLOPT_PROXY, _))
      .WillOnce(Return(CURLE_OK));

  Request request("http://example.org");

  EXPECT_TRUE(curl_client_->NewHandle(&request) != nullptr);
}

TEST_F(CurlHttpClientTest, StartNewRequestsOk) {
  SetInitializeExpectations();
  EXPECT_TRUE(curl_client_->Initialize());
  EXPECT_EQ(0, curl_client_->schedule_queue_size());

  SetScheduleExpectations();
  Request request_("http://example.org");
  EXPECT_TRUE(curl_client_->Schedule(&request_));
  ASSERT_EQ(1, curl_client_->schedule_queue_size());

  EXPECT_CALL(*mock_rate_limiter_, TakeRateSlot()).WillOnce(Return(true));

  EXPECT_EQ(0, curl_client_->running_handles_size());
  EXPECT_TRUE(curl_client_->StartNewRequests());
  EXPECT_EQ(1, curl_client_->running_handles_size());
}

TEST_F(CurlHttpClientTest, StartNewRequestsHitsConnectionLimit) {
  absl::SetFlag(&FLAGS_connection_limit, -1);

  SetInitializeExpectations();
  EXPECT_TRUE(curl_client_->Initialize());

  Request request_("http://example.org");
  EXPECT_TRUE(curl_client_->Schedule(&request_));
  ASSERT_EQ(1, curl_client_->schedule_queue_size());

  EXPECT_EQ(0, curl_client_->running_handles_size());
  EXPECT_TRUE(curl_client_->StartNewRequests());
  EXPECT_EQ(0, curl_client_->running_handles_size());
  curl_easy_cleanup(curl_);
}

TEST_F(CurlHttpClientTest, StartNewRequestsFailOnWriteCallback) {
  SetInitializeExpectations();
  EXPECT_TRUE(curl_client_->Initialize());
  EXPECT_CALL(*mock_curl_, EasyInit()).WillOnce(Return(curl_));
  EXPECT_CALL(*mock_curl_, EasySetOpt(_, _, _))
      .WillRepeatedly(Return(CURLE_OK));
  EXPECT_CALL(*mock_curl_,
              EasySetWriteCallback(curl_, CURLOPT_WRITEFUNCTION, _))
      .WillOnce(Return(CURLE_OUT_OF_MEMORY));

  Request request_("http://example.org");
  EXPECT_TRUE(curl_client_->Schedule(&request_));
  ASSERT_EQ(1, curl_client_->schedule_queue_size());

  EXPECT_CALL(*mock_rate_limiter_, TakeRateSlot()).WillOnce(Return(true));

  EXPECT_EQ(0, curl_client_->running_handles_size());
  EXPECT_FALSE(curl_client_->StartNewRequests());
  EXPECT_EQ(0, curl_client_->running_handles_size());
}

TEST_F(CurlHttpClientTest, StartNewRequestsOkWithHeaders) {
  SetInitializeExpectations();
  EXPECT_TRUE(curl_client_->Initialize());

  SetNewHandleExpectations();
  std::string header_name("header");
  std::string header_value("value");
  Request request_("http://example.org");
  request_.SetHeader(header_name, header_value, true);
  EXPECT_TRUE(curl_client_->Schedule(&request_));
  ASSERT_EQ(1, curl_client_->schedule_queue_size());

  EXPECT_CALL(*mock_curl_, MultiAddHandle(curlm_, _))
      .WillOnce(Return(CURLM_OK));
  EXPECT_CALL(*mock_curl_, MultiPerform(curlm_, _))
      .WillRepeatedly(Return(CURLM_OK));
  EXPECT_CALL(*mock_rate_limiter_, TakeRateSlot()).WillOnce(Return(true));
  EXPECT_CALL(*mock_curl_, AppendSlist(_, StrEq("User-Agent: Plusfish")))
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_curl_, AppendSlist(_, StrEq("header: value")))
      .WillOnce(Return(true));

  EXPECT_EQ(0, curl_client_->running_handles_size());
  EXPECT_TRUE(curl_client_->StartNewRequests());
  EXPECT_EQ(1, curl_client_->running_handles_size());
}

TEST_F(CurlHttpClientTest, StartNewRequestsOkWithDomainSpecificHeaders) {
  SetInitializeExpectations();
  EXPECT_TRUE(curl_client_->Initialize());

  SetNewHandleExpectations();
  std::string header_name("header");
  std::string header_value("value");
  std::string domain("example.org");
  Request request_("http://" + domain);
  EXPECT_TRUE(curl_client_->Schedule(&request_));
  ASSERT_EQ(1, curl_client_->schedule_queue_size());

  EXPECT_CALL(*mock_curl_, MultiAddHandle(curlm_, _))
      .WillOnce(Return(CURLM_OK));
  EXPECT_CALL(*mock_curl_, MultiPerform(curlm_, _))
      .WillRepeatedly(Return(CURLM_OK));
  EXPECT_CALL(*mock_rate_limiter_, TakeRateSlot()).WillOnce(Return(true));
  EXPECT_CALL(*mock_curl_, AppendSlist(_, _)).WillOnce(Return(true));
  EXPECT_CALL(*mock_curl_, AppendSlist(_, HasSubstr(header_name)))
      .WillOnce(Return(true));

  // Set the header for the matching domain
  EXPECT_TRUE(
      curl_client_->RegisterDefaultHeader(domain, header_name, header_value));
  // Set the header for a different domain
  EXPECT_TRUE(curl_client_->RegisterDefaultHeader("foo.org", header_name,
                                                  header_value));

  EXPECT_EQ(0, curl_client_->running_handles_size());
  EXPECT_TRUE(curl_client_->StartNewRequests());
  EXPECT_EQ(1, curl_client_->running_handles_size());
}

TEST_F(CurlHttpClientTest, StartNewRequestsFailsWithHeaders) {
  SetInitializeExpectations();
  EXPECT_TRUE(curl_client_->Initialize());

  SetNewHandleExpectations();
  std::string header_name("header");
  std::string header_value("value");
  Request request_("http://example.org");
  request_.SetHeader(header_name, header_value, true);
  EXPECT_TRUE(curl_client_->Schedule(&request_));
  ASSERT_EQ(1, curl_client_->schedule_queue_size());

  EXPECT_CALL(*mock_rate_limiter_, TakeRateSlot()).WillOnce(Return(true));
  EXPECT_CALL(*mock_curl_, AppendSlist(_, _)).WillOnce(Return(false));

  EXPECT_EQ(0, curl_client_->running_handles_size());
  EXPECT_FALSE(curl_client_->StartNewRequests());
  EXPECT_EQ(0, curl_client_->running_handles_size());
}

TEST_F(CurlHttpClientTest, StartNewRequestsHitsRateLimit) {
  SetInitializeExpectations();
  EXPECT_TRUE(curl_client_->Initialize());

  // Ratelimiter says no.
  EXPECT_CALL(*mock_rate_limiter_, TakeRateSlot()).WillOnce(Return(false));

  Request request_("http://example.org");
  EXPECT_TRUE(curl_client_->Schedule(&request_));
  ASSERT_EQ(1, curl_client_->schedule_queue_size());
  EXPECT_TRUE(curl_client_->StartNewRequests());
  EXPECT_EQ(0, curl_client_->running_handles_size());
  curl_easy_cleanup(curl_);
}

TEST_F(CurlHttpClientTest, StartNewRequestsDisabled) {
  curl_client_->Disable();
  EXPECT_FALSE(curl_client_->StartNewRequests());
  curl_easy_cleanup(curl_);
}

}  // namespace plusfish
