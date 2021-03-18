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

#include "response.h"

#include "gtest/gtest.h"
#include "proto/http_common.pb.h"
#include "proto/http_response.pb.h"
#include "util/html_fingerprint.h"

namespace plusfish {

class ResponseTest : public ::testing::Test {
 protected:
  ResponseTest() {}

  Response response_;
};

TEST_F(ResponseTest, ParsesResponseOk) {
  std::string test_response("HTTP/1.0 200 OK\r\nHeader: Value\r\n\r\nBody");
  ASSERT_TRUE(response_.Parse(test_response));
  ASSERT_EQ(response_.body(), "Body");
}

TEST_F(ResponseTest, ParsesResponseNoCrOk) {
  std::string test_response("HTTP/1.0 200 OK\nA: B\nC: D\n\nBody");
  ASSERT_TRUE(response_.Parse(test_response));
  const std::string* header_value_one = response_.GetHeader("A");
  EXPECT_STREQ(header_value_one->c_str(), "B");
  const std::string* header_value_two = response_.GetHeader("C");
  EXPECT_STREQ(header_value_two->c_str(), "D");
  EXPECT_EQ(response_.body(), "Body");
}

TEST_F(ResponseTest, ParsesResponseNoBodyOK) {
  std::string test_response("HTTP/1.0 200 OK\r\nHeader: Value\r\n\r\n");
  ASSERT_TRUE(response_.Parse(test_response));
  ASSERT_EQ(response_.body(), "");
}

TEST_F(ResponseTest, ParsesResponseNoNewlines) {
  std::string test_response("HTTP/1.0 200 OK\r\nHeader: Value");
  ASSERT_FALSE(response_.Parse(test_response));
}

TEST_F(ResponseTest, ParsesResponseHeaderWithoutContent) {
  std::string test_response("HTTP/1.0 200 OK\r\nHeader:\r\n\r\nboo");
  ASSERT_TRUE(response_.Parse(test_response));
}

TEST_F(ResponseTest, ParsesResponseEmpty) {
  std::string test_response("");
  ASSERT_FALSE(response_.Parse(test_response));
}

TEST_F(ResponseTest, ParsesResponseNotHTTP) {
  std::string test_response("200-FTP SERVER");
  ASSERT_FALSE(response_.Parse(test_response));
}

TEST_F(ResponseTest, ParsesGetHeaderOk) {
  std::string test_response("HTTP/1.0 200 OK\r\nA: B\r\nC: D\r\n\r\nBody");
  ASSERT_TRUE(response_.Parse(test_response));
  const std::string* header_value_one = response_.GetHeader("A");
  EXPECT_STREQ(header_value_one->c_str(), "B");
  const std::string* header_value_two = response_.GetHeader("C");
  EXPECT_STREQ(header_value_two->c_str(), "D");
}

TEST_F(ResponseTest, ParsesGetHeaderIsCaseInsensitive) {
  std::string test_response("HTTP/1.0 200 OK\r\nHeAdeR: Value\r\n\r\nBody");
  ASSERT_TRUE(response_.Parse(test_response));
  const std::string* header_value = response_.GetHeader("header");
  EXPECT_STREQ(header_value->c_str(), "Value");
}

TEST_F(ResponseTest, ParsesGetHeaderNoValue) {
  std::string test_response("HTTP/1.0 200 OK\r\nHeader\r\n\r\nBody");
  ASSERT_TRUE(response_.Parse(test_response));
  const std::string* header_value = response_.GetHeader("Header");
  ASSERT_EQ(header_value, nullptr);
}

TEST_F(ResponseTest, ParsesGetNonExistingHeaderOk) {
  std::string test_response("HTTP/1.0 200 OK\r\nHeader: Value\r\n\r\nBody");
  ASSERT_TRUE(response_.Parse(test_response));
  const std::string* header_value = response_.GetHeader("Foo");
  ASSERT_EQ(header_value, nullptr);
}

TEST_F(ResponseTest, ParsesResponseCodeOk) {
  std::string test_response("HTTP/1.0 200 OK\r\nHeader: Value\r\n\r\nBody");
  ASSERT_TRUE(response_.Parse(test_response));
  ASSERT_EQ(response_.proto().code(), HttpResponse::OK);
}

TEST_F(ResponseTest, ParsesResponseCodeNotKnown) {
  std::string test_response("HTTP/1.0 666 OK\r\nHeader: Value\r\n\r\nBody");
  ASSERT_FALSE(response_.Parse(test_response));
}

TEST_F(ResponseTest, ParsesResponseCodeInvalid) {
  std::string test_response("HTTP/1.0 WWW OK\r\nHeader: Value\r\n\r\nBody");
  ASSERT_FALSE(response_.Parse(test_response));
}

TEST_F(ResponseTest, ParsesResponseCodeNotFound) {
  std::string test_response(
      "HTTP/1.0 404 Not Found\r\nHeader: Value\r\n\r\nBody");
  ASSERT_TRUE(response_.Parse(test_response));
  ASSERT_EQ(response_.proto().code(), 404);
}

TEST_F(ResponseTest, ParsesResponseStatusLineIncomplete) {
  std::string test_response("HTTP/1.0\r\nHeader: Value\r\n\r\nBody");
  ASSERT_FALSE(response_.Parse(test_response));
}

TEST_F(ResponseTest, ParsesResponseMimeOK) {
  std::string test_response(
      "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\nHi");
  ASSERT_TRUE(response_.Parse(test_response));
  ASSERT_EQ(response_.proto().mime_type(), MimeInfo::ASC_HTML);
}

TEST_F(ResponseTest, ParsesResponseHtmlEqualOK) {
  std::unique_ptr<HtmlFingerprint> fp(new HtmlFingerprint());
  std::string test_response(
      "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\nHi");
  response_.set_html_fingerprint(std::move(fp));
  ASSERT_TRUE(response_.Parse(test_response));
  ASSERT_TRUE(response_.Equals(response_));
}

TEST_F(ResponseTest, ParsesResponseHtmlDifferentFingerprint) {
  std::unique_ptr<HtmlFingerprint> fp(new HtmlFingerprint());
  std::unique_ptr<HtmlFingerprint> fp2(new HtmlFingerprint());

  fp2->AddWord("one");
  fp2->AddWord("two");
  fp2->AddWord("three");
  std::string test_response(
      "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\nHi");

  Response response2;

  response_.set_html_fingerprint(std::move(fp));
  response2.set_html_fingerprint(std::move(fp2));
  ASSERT_TRUE(response_.Parse(test_response));
  ASSERT_TRUE(response2.Parse(test_response));
  ASSERT_FALSE(response_.Equals(response2));
}

TEST_F(ResponseTest, ParsesResponseHtmlMissingFingerprint) {
  std::unique_ptr<HtmlFingerprint> fp(new HtmlFingerprint());

  std::string test_response(
      "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\nHi");

  Response response2;
  response_.set_html_fingerprint(std::move(fp));
  ASSERT_TRUE(response_.Parse(test_response));
  ASSERT_TRUE(response2.Parse(test_response));
  ASSERT_FALSE(response_.Equals(response2));
}

TEST_F(ResponseTest, ParsesBinaryResponseEqualOK) {
  std::string test_response(
      "HTTP/1.0 200 OK\r\nContent-Type: application/zip\r\n\r\n\x41\x41");
  ASSERT_TRUE(response_.Parse(test_response));
  ASSERT_TRUE(response_.Equals(response_));
}

TEST_F(ResponseTest, ParsesResponseEqualsGetsMimeDifference) {
  std::string test_response_one(
      "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\nHi");
  std::string test_response_two(
      "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\n\r\nHi");
  Response response_one;
  Response response_two;
  ASSERT_TRUE(response_one.Parse(test_response_one));
  ASSERT_TRUE(response_two.Parse(test_response_two));
  ASSERT_FALSE(response_one.Equals(response_two));
}

TEST_F(ResponseTest, ParsesResponseEqualsGetsCodeDifference) {
  Response response_one;
  Response response_two;
  std::string test_response_one(
      "HTTP/1.0 302 OK\r\nContent-Type: text/html\r\n\r\nHi");
  std::string test_response_two(
      "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\nHi");
  ASSERT_TRUE(response_one.Parse(test_response_one));
  ASSERT_TRUE(response_two.Parse(test_response_two));
  ASSERT_FALSE(response_one.Equals(response_two));
}

TEST_F(ResponseTest, ParsesBinaryResponseEqualsGetsBodyDifference) {
  std::string test_response_one(
      "HTTP/1.0 200 OK\r\nContent-Type: application/zip\r\n\r\n\x41");
  std::string test_response_two(
      "HTTP/1.0 200 OK\r\nContent-Type: application/zip\r\n\r\n\x42");
  Response response_one;
  Response response_two;
  ASSERT_TRUE(response_one.Parse(test_response_one));
  ASSERT_TRUE(response_two.Parse(test_response_two));
  ASSERT_FALSE(response_one.Equals(response_two));
}

TEST_F(ResponseTest, ParsesResponseUnknownMimeOK) {
  std::string test_response(
      "HTTP/1.0 200 OK\r\nContent-Type: text/foo\r\n\r\nHi");
  ASSERT_TRUE(response_.Parse(test_response));
  ASSERT_EQ(response_.proto().mime_type(), MimeInfo::UNKNOWN_MIME);
}

TEST_F(ResponseTest, ParsesResponseMimeAttributeRecognizedOk) {
  std::string test_response(
      "HTTP/1.0 200 OK\r\nContent-Type: text/html; "
      "charset=utf-8;\r\n\r\nHi");
  ASSERT_TRUE(response_.Parse(test_response));
  ASSERT_EQ(response_.proto().mime_type(), MimeInfo::ASC_HTML);
}

}  // namespace plusfish
