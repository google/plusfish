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

#include "crawler.h"

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "http_client.h"
#include "proto/http_response.pb.h"
#include "request.h"
#include "response.h"
#include "testing/datastore_mock.h"
#include "testing/http_client_mock.h"
#include "testing/reporter_mock.h"
#include "testing/request_mock.h"
#include "testing/selective_auditor_mock.h"

ABSL_DECLARE_FLAG(bool, extract_links);

using testing::_;
using testing::Return;

namespace plusfish {

class CrawlerTest : public ::testing::Test {
 public:
  CrawlerTest() : http_client_() {}

  void SetUp() override {
    crawler_.reset(new Crawler(&http_client_, &auditor_, nullptr, &datastore_));
  }

  std::unique_ptr<Crawler> crawler_;
  testing::MockHttpClient http_client_;
  testing::MockDataStore datastore_;
  testing::MockSelectiveAuditor auditor_;
};

TEST_F(CrawlerTest, ScheduleOKTest) {
  Request request_one("http://example.org/");
  EXPECT_CALL(http_client_, Schedule(&request_one)).WillOnce(Return(true));
  EXPECT_TRUE(crawler_->Crawl(&request_one));
}

TEST_F(CrawlerTest, ScheduleButClientRejects) {
  Request request("http://example.org/");
  EXPECT_CALL(http_client_, Schedule(&request)).WillOnce(Return(false));
  EXPECT_FALSE(crawler_->Crawl(&request));
}

TEST_F(CrawlerTest, RequestCallbackNoResponse) {
  testing::MockRequest mock_request("http://foo");
  EXPECT_CALL(mock_request, response()).WillOnce(Return(nullptr));
  EXPECT_EQ(1, crawler_->RequestCallback(&mock_request));
}

TEST_F(CrawlerTest, RequestCallbackOk) {
  testing::MockRequest request("http://foo");

  Response response;
  response.Parse(
      "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
      "<html><title>this</title><p>is cool stuff</p></html>");

  std::unique_ptr<HtmlFingerprint> response_fp =
      absl::make_unique<HtmlFingerprint>();
  response_fp->AddWord("1");
  response_fp->AddWord("1");
  response.set_html_fingerprint(std::move(response_fp));

  EXPECT_CALL(request, response()).WillRepeatedly(Return(&response));

  EXPECT_CALL(request, parent_id()).WillRepeatedly(Return(-1));
  EXPECT_CALL(datastore_, AddRequestToAuditQueue(&request)).Times(1);
  EXPECT_EQ(0, crawler_->RequestCallback(&request));
}

TEST_F(CrawlerTest, RequestCallbackNoAuditorOk) {
  // Initialize without an auditor.
  crawler_.reset(new Crawler(&http_client_, &datastore_));
  testing::MockRequest request("http://foo");

  Response response;
  response.Parse(
      "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
      "<html><title>this</title><p>is cool stuff</p></html>");

  std::unique_ptr<HtmlFingerprint> response_fp =
      absl::make_unique<HtmlFingerprint>();
  response_fp->AddWord("1");
  response_fp->AddWord("1");
  response.set_html_fingerprint(std::move(response_fp));

  EXPECT_CALL(request, response()).WillRepeatedly(Return(&response));
  EXPECT_CALL(request, parent_id()).WillRepeatedly(Return(-1));
  EXPECT_CALL(datastore_, AddRequestToAuditQueue(&request)).Times(0);
  EXPECT_EQ(0, crawler_->RequestCallback(&request));
}
TEST_F(CrawlerTest, RequestCallbackExtractLinksDisabled) {
  absl::SetFlag(&FLAGS_extract_links, false);

  testing::MockRequest request("http://foo");
  testing::MockRequest parent("http://foo/bar");
  testing::MockRequest grandparent("http://foo/rab/");

  Response response;
  response.Parse(
      "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
      "<html><title>this</title><p>is cool stuff</p></html>");
  Response parent_response;
  parent_response.Parse(
      "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
      "<html>this<p>a b c</p></html>");

  Response grandparent_response;
  grandparent_response.Parse(
      "HTTP/1.0 200 OK\r\nContent-Type: image/gif\r\n\r\n"
      "GIF");

  std::unique_ptr<HtmlFingerprint> response_fp =
      absl::make_unique<HtmlFingerprint>();
  response_fp->AddWord("1");
  response_fp->AddWord("1");
  response.set_html_fingerprint(std::move(response_fp));

  std::unique_ptr<HtmlFingerprint> parent_fp =
      absl::make_unique<HtmlFingerprint>();
  parent_fp->AddWord("11");
  parent_fp->AddWord("11");
  parent_response.set_html_fingerprint(std::move(parent_fp));

  std::unique_ptr<HtmlFingerprint> grandparent_fp =
      absl::make_unique<HtmlFingerprint>();
  grandparent_fp->AddWord("111");
  grandparent_fp->AddWord("111");
  grandparent_response.set_html_fingerprint(std::move(grandparent_fp));

  EXPECT_CALL(request, response()).WillRepeatedly(Return(&response));
  EXPECT_CALL(parent, response()).WillRepeatedly(Return(&parent_response));
  EXPECT_CALL(grandparent, response())
      .WillRepeatedly(Return(&grandparent_response));

  EXPECT_CALL(request, parent_id()).WillRepeatedly(Return(43));
  EXPECT_CALL(parent, parent_id()).WillRepeatedly(Return(44));
  EXPECT_CALL(datastore_, GetRequestById(42)).WillRepeatedly(Return(&request));
  EXPECT_CALL(datastore_, GetRequestById(43)).WillRepeatedly(Return(&parent));
  EXPECT_CALL(datastore_, GetRequestById(44))
      .WillRepeatedly(Return(&grandparent));

  EXPECT_CALL(datastore_, AddRequestToAuditQueue(&request)).Times(1);
  EXPECT_EQ(0, crawler_->RequestCallback(&request));
}

TEST_F(CrawlerTest, RequestCallbackSchedulesNoTestOnHttpErrorCode) {
  absl::SetFlag(&FLAGS_extract_links, false);

  testing::MockRequest request("http://foo");
  testing::MockRequest parent("http://foo/bar");
  testing::MockRequest grandparent("http://foo/rab/");

  Response response;
  response.Parse(
      "HTTP/1.0 404 OK\r\nContent-Type: text/html\r\n\r\n"
      "<html><title>this</title><p>is cool stuff</p></html>");

  std::unique_ptr<HtmlFingerprint> response_fp =
      absl::make_unique<HtmlFingerprint>();
  response_fp->AddWord("1");
  response_fp->AddWord("1");
  response.set_html_fingerprint(std::move(response_fp));

  EXPECT_CALL(request, response()).WillRepeatedly(Return(&response));
  EXPECT_CALL(request, parent_id()).WillRepeatedly(Return(-1));

  EXPECT_CALL(datastore_, AddRequestToAuditQueue(&request)).Times(0);
  EXPECT_EQ(0, crawler_->RequestCallback(&request));
}

TEST_F(CrawlerTest, RequestCallbackSchedulesNoTestOn404FingerprintMatch) {
  absl::SetFlag(&FLAGS_extract_links, false);

  testing::MockRequest request("http://foo");
  testing::MockRequest parent("http://foo/bar");
  testing::MockRequest grandparent("http://foo/rab/");

  Response response;
  response.Parse(
      "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
      "<html><title>this</title><p>is cool stuff</p></html>");

  std::unique_ptr<HtmlFingerprint> response_fp =
      absl::make_unique<HtmlFingerprint>();
  response_fp->AddWord("1");
  response_fp->AddWord("1");
  response.set_html_fingerprint(std::move(response_fp));

  EXPECT_CALL(request, response()).WillRepeatedly(Return(&response));
  EXPECT_CALL(request, parent_id()).WillRepeatedly(Return(-1));
  EXPECT_CALL(datastore_,
              IsFileNotFoundHtmlFingerprint(response.get_html_fingerprint()))
      .WillOnce(Return(true));

  EXPECT_CALL(datastore_, AddRequestToAuditQueue(&request)).Times(0);
  EXPECT_EQ(0, crawler_->RequestCallback(&request));
}

TEST_F(CrawlerTest, RequestCallbackDetectedDuplicateBinaryResponse) {
  absl::SetFlag(&FLAGS_extract_links, false);
  Response response;
  response.Parse(
      "HTTP/1.0 200 OK\r\nContent-Type: image/gif\r\n\r\n"
      "This this this");
  testing::MockRequest mock_request("http://foo");
  EXPECT_CALL(mock_request, response()).WillRepeatedly(Return(&response));
  EXPECT_CALL(mock_request, parent_id()).WillRepeatedly(Return(42));
  EXPECT_CALL(datastore_, GetRequestById(42))
      .WillRepeatedly(Return(&mock_request));
  EXPECT_EQ(1, crawler_->RequestCallback(&mock_request));
}

TEST_F(CrawlerTest, RequestCallbackDetectedChecksParentBinaryResponse) {
  absl::SetFlag(&FLAGS_extract_links, false);
  Response response;
  response.Parse(
      "HTTP/1.0 200 OK\r\nContent-Type: image/gif\r\n\r\n"
      "This this this");
  Response parent_response;
  parent_response.Parse(
      "HTTP/1.0 200 OK\r\nContent-Type: image/gif\r\n\r\n"
      "That that that");
  testing::MockRequest mock_request("http://foo");
  testing::MockRequest parent_mock_request("http://foo/bar");
  EXPECT_CALL(mock_request, response()).WillRepeatedly(Return(&response));
  EXPECT_CALL(parent_mock_request, response())
      .WillRepeatedly(Return(&parent_response));
  EXPECT_CALL(mock_request, parent_id()).WillRepeatedly(Return(42));
  EXPECT_CALL(datastore_, GetRequestById(42))
      .WillOnce(Return(&parent_mock_request));
  EXPECT_CALL(mock_request, truncate_response_body());
  // Since they are not the same the request is added successfully.
  EXPECT_EQ(0, crawler_->RequestCallback(&mock_request));
}

TEST_F(CrawlerTest, RequestCallbackTruncatesResponse) {
  absl::SetFlag(&FLAGS_extract_links, false);
  Response response;
  response.Parse(
      "HTTP/1.0 200 OK\r\nContent-Type: image/gif\r\n\r\n"
      "Owthiswillbetrunctatedduetothemimetype");
  testing::MockRequest mock_request("http://foo");
  EXPECT_CALL(mock_request, response()).WillRepeatedly(Return(&response));
  EXPECT_CALL(mock_request, truncate_response_body());
  crawler_->RequestCallback(&mock_request);
}

TEST_F(CrawlerTest, ScrapeEmptyResponseOk) {
  absl::SetFlag(&FLAGS_extract_links, true);
  testing::MockRequest mock_request("http://foo");
  EXPECT_CALL(mock_request, response()).WillOnce(Return(nullptr));
  EXPECT_FALSE(crawler_->Scrape(&mock_request));
}

TEST_F(CrawlerTest, ScrapeResponseOk) {
  absl::SetFlag(&FLAGS_extract_links, true);
  Response response;
  response.Parse("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n");
  testing::MockRequest mock_request("http://foo");
  EXPECT_CALL(mock_request, response()).WillRepeatedly(Return(&response));
  EXPECT_CALL(datastore_, AddResponseFingerprintToRequest(mock_request.id(), _))
      .WillOnce(Return(true));
  EXPECT_TRUE(crawler_->ScrapeWithLinks(&mock_request));
}

TEST_F(CrawlerTest, RequestCallbackResponseWithNoHtmlMime) {
  HttpResponse response_proto;
  response_proto.set_mime_type(MimeInfo_MimeType_AV_QT);
  Response empty_response(response_proto);
  testing::MockRequest mock_request("http://foo");
  EXPECT_CALL(mock_request, response()).WillRepeatedly(Return(&empty_response));
  EXPECT_EQ(0, crawler_->RequestCallback(&mock_request));
}

}  // namespace plusfish
