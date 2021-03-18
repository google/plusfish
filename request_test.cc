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

#include "request.h"

#include <string.h>

#include <string>

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "proto/http_request.pb.h"
#include "request_handler.h"
#include "testing/request_mock.h"
#include "testing/security_check_mock.h"

namespace plusfish {

class MockRequestHandler : public RequestHandlerInterface {
 public:
  MOCK_METHOD1(RequestCallback, int(Request* req));
};

class RequestTest : public ::testing::Test {
 protected:
  RequestTest() : request_(), mock_request_handler_() {}
  Request request_;
  MockRequestHandler mock_request_handler_;
};

TEST_F(RequestTest, ParseSimpleUrlTest) {
  std::string url = "http://www.google.com:80/aa/bb/cc/";
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  EXPECT_STREQ(request_.host().c_str(), "www.google.com");
  EXPECT_STREQ(request_.path().c_str(), "/aa/bb/cc/");
  EXPECT_EQ(request_.proto().url(), url);
}

TEST_F(RequestTest, RequestFromProtoTest) {
  std::string url = "http://www.google.com/aa/bb/cc/";
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  Request from_proto(request_.proto());
  EXPECT_STREQ(from_proto.host().c_str(), "www.google.com");
}

TEST_F(RequestTest, UrlPathElementsTest) {
  std::string url = "https://www.google.com/aa/bb/cc/";
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  EXPECT_EQ(request_.proto().path_size(), 4);
  EXPECT_STREQ(request_.proto().path(0).value().c_str(), "aa");
}

TEST_F(RequestTest, EncodedUrlTest) {
  std::string url = "https://www.google.com/%61%61/bb%2fcc/";
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  EXPECT_EQ(request_.proto().path_size(), 3);
  EXPECT_STREQ(request_.proto().path(0).value().c_str(), "aa");
}

TEST_F(RequestTest, ParseSemiComplexUrlTest) {
  std::string url = "https://www.google.com:80/aa/bb/cc/?first=value&ff=#frag";
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  const HttpRequest& proto = request_.proto();
  // HttpRequest proto expections.
  EXPECT_EQ(proto.param_size(), 2);
  EXPECT_TRUE(proto.ssl());
  EXPECT_TRUE(proto.param(0).has_value());
  EXPECT_STREQ(proto.param(0).name().c_str(), "first");
  EXPECT_STREQ(proto.param(0).value().c_str(), "value");
  EXPECT_STREQ(url.c_str(), url.c_str());
}

TEST_F(RequestTest, RequestWithDifferentParamValuesIsConsideredEqualTest) {
  std::string url1 = "http://www.google.com:80/aa/bb/cc/?dd=11&ff=#frag";
  std::string url2 = "http://www.google.com:80/aa/bb/cc/?dd=ee&ff#frag";

  Request with_equal(url1);
  Request without_equal(url2);

  EXPECT_TRUE(with_equal.Equals(without_equal));
}

TEST_F(RequestTest, RequestWithDifferentMethodIsNotEqual) {
  std::string url1 = "http://www.google.com:80/aa/bb/cc/?dd=ee";
  std::string url2 = "http://www.google.com:80/aa/bb/cc/?dd=ee";

  Request get_request(url1);
  Request post_request(url2);
  post_request.SetPostParameter("name", "value", false /* replace */);

  EXPECT_FALSE(get_request.Equals(post_request));
}

TEST_F(RequestTest, RequestWithBodyParametersIsEqual) {
  std::string url1 = "http://www.google.com:80/aa/bb/cc/?dd=ee";
  std::string url2 = "http://www.google.com:80/aa/bb/cc/?dd=ee";

  Request request_one(url1);
  Request request_two(url2);
  request_one.SetPostParameter("name", "this is", false /* replace */);
  request_two.SetPostParameter("name", "ignored", false /* replace */);
  EXPECT_TRUE(request_one.Equals(request_two));
}

TEST_F(RequestTest, EmptyParamValueResultsInEmptyStringTest) {
  std::string url = "http://www.google.com:80/aa/bb/cc/?dd=ee&empty=#frag";
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));

  // With equal has an empty value.
  const HttpRequest& proto = request_.proto();
  EXPECT_EQ(proto.param_size(), 2);
  EXPECT_TRUE(proto.param(1).has_value());
  EXPECT_STREQ(proto.param(1).name().c_str(), "empty");
  EXPECT_STREQ(proto.param(1).value().c_str(), "");
}

TEST_F(RequestTest, ParamWithoutEqualSignAndValueTest) {
  std::string url = "http://www.google.com:80/aa/bb/cc/?dd=ee&debug#frag";
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));

  // With equal has an empty value.
  const HttpRequest& proto = request_.proto();
  EXPECT_EQ(proto.param_size(), 2);
  EXPECT_FALSE(proto.param(1).has_value());
  EXPECT_STREQ(proto.param(1).name().c_str(), "debug");
}

TEST_F(RequestTest, RequestUrlIsValid) {
  std::string url = "http://www.google.com:80/aa/";

  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  EXPECT_TRUE(request_.url_is_valid());
}

TEST_F(RequestTest, RequestHandlerDoneCbTestWithoutResponse) {
  std::string url = "http://www.google.com:80/aa/";

  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  request_.set_request_handler(&mock_request_handler_);
  request_.DoneCb();
  EXPECT_EQ(request_.response(), nullptr);
}

TEST_F(RequestTest, RequestHandlerDoneCbTestWithResponse) {
  std::string url = "http://www.google.com:80/aa/";
  std::string response = "HTTP/1.0 200 OK\r\n\r\nHello";

  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  request_.set_request_handler(&mock_request_handler_);
  EXPECT_CALL(mock_request_handler_, RequestCallback(&request_)).Times(1);
  request_.ResponseCb(response.c_str(), response.size());
  request_.DoneCb();
}

TEST_F(RequestTest, ParseRelativeUrlTest) {
  std::string url1 = "http://www.google.com:80/aa/";
  std::string url2 = "/bb/";
  std::string url3 = "http://www.google.com:80/bb/";
  std::string url4 = "bb/";
  std::string url5 = "http://www.google.com:80/aa/bb/";
  std::string url6 = "http://www.google.com:80/aa/index.html";

  Request one(url1);
  Request two(url2, &one);
  Request three(url4, &one);
  Request four(url6);
  Request five(url4, &four);
  // Relative to the host
  EXPECT_EQ(two.url(), url3);
  // Relative to a path
  EXPECT_EQ(three.url(), url5);
  // Relative to a path with file
  EXPECT_EQ(five.url(), url5);
}

TEST_F(RequestTest, CompareTest) {
  std::string url1 = "http://www.google.com/aa/bb/";
  std::string url2 = "http://www.google.com/aa/bb/?aa=bb&cc=dd";
  std::string url3 = "http://www.google.com/aa/bb/?cc=dd&aa=bb";
  std::string url4 = "http://www.google.com/bb/aa/?cc=dd&aa=bb";

  Request one(url1);
  Request two(url2);
  Request three(url3);
  Request four(url4);

  // Same object
  EXPECT_TRUE(one.Equals(one));
  // Different URL
  EXPECT_FALSE(one.Equals(two));
  // Same URL, different parameter order
  EXPECT_TRUE(two.Equals(three));
  // Similar URL, different path order
  EXPECT_FALSE(three.Equals(four));
}

TEST_F(RequestTest, ToUrlTest) {
  std::string url = "http://www.google.com:80/aa/bb/cc?aa=bb&debug";
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  EXPECT_EQ(request_.url(), url);
}

TEST_F(RequestTest, GetPortTest) {
  std::string url = "http://www.google.com/aa/bb/cc?aa=bb&debug";
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  EXPECT_EQ(request_.port(), 80);
}

TEST_F(RequestTest, AddResponseStringTest) {
  std::string url = "http://www.google.com/aa/bb/cc?aa=bb&debug";
  std::string response("123");
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  request_.ResponseCb(response.c_str(), 3);
  EXPECT_EQ(response, request_.raw_response());
}

TEST_F(RequestTest, AddGetParameterTest) {
  std::string url = "http://www.google.com/aa/bb/cc?aa=bb";
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  EXPECT_EQ(request_.proto().param_size(), 1);
  EXPECT_EQ(1, request_.SetGetParameter("cc", "dd", false));
  EXPECT_EQ(request_.proto().param_size(), 2);
}

TEST_F(RequestTest, AddHeaderTest) {
  std::string url = "http://www.google.com/";
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  std::string header_name("my");
  std::string header_value("value");
  EXPECT_EQ(1, request_.SetHeader(header_name, header_value, false));
  EXPECT_EQ(request_.proto().header_size(), 2);

  // Second value with replace=false will not be set.
  std::string second_header_value("second_value");
  EXPECT_EQ(0, request_.SetHeader(header_name, second_header_value, false));
  ASSERT_EQ(request_.proto().header_size(), 2);

  EXPECT_EQ(request_.proto().header(1).name(), header_name);
  EXPECT_EQ(request_.proto().header(1).value(), header_value);
}

TEST_F(RequestTest, AddGetParameterReplaceExisting) {
  std::string url = "http://www.google.com/aa/bb/cc?aa=bb";
  std::string test_value = "42";
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  EXPECT_EQ(1, request_.SetGetParameter("aa", test_value, true));
  EXPECT_EQ(request_.proto().param_size(), 1);
  EXPECT_EQ(request_.proto().param(0).value(), test_value);
}

TEST_F(RequestTest, AddPostParameterTest) {
  std::string url = "http://www.google.com/aa/bb/cc";
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  EXPECT_EQ(request_.proto().body_param_size(), 0);
  EXPECT_EQ(1, request_.SetPostParameter("cc", "dd", false));
  EXPECT_EQ(request_.proto().body_param_size(), 1);
}

TEST_F(RequestTest, AddPostParameterReplaceExisting) {
  std::string url = "http://www.google.com/aa/bb/cc?aa=bb";
  std::string initial_value = "42";
  std::string replacement_value = "42";
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  EXPECT_EQ(1, request_.SetPostParameter("aa", initial_value, true));
  EXPECT_EQ(1, request_.SetPostParameter("aa", replacement_value, true));
  EXPECT_EQ(request_.proto().body_param_size(), 1);
  EXPECT_EQ(request_.proto().body_param(0).value(), replacement_value);
  EXPECT_EQ(request_.proto().method(), HttpRequest_RequestMethod_POST);
}

TEST_F(RequestTest, GetRequestBodyTest) {
  std::string url = "http://www.google.com/aa/bb/cc?aa=bb";
  std::string test_value = "42";
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  request_.SetPostParameter("aa", test_value, true);
  request_.SetPostParameter("bb", test_value, true);
  EXPECT_STREQ(request_.GetRequestBody().c_str(), "aa=42&bb=42");
}

TEST_F(RequestTest, GetRequestBodyEmptyTest) {
  std::string url = "http://www.google.com/aa/bb/cc?aa=bb";
  EXPECT_TRUE(request_.ParseUrl(url, nullptr));
  EXPECT_STREQ(request_.GetRequestBody().c_str(), "");
}

TEST_F(RequestTest, RequestOriginIsSet) {
  std::string origin_url = "http://www.google.com:80/";
  std::string new_url = "http://www.google.com:80/aa/";
  Request origin(origin_url);
  Request new_request(new_url, &origin);
  EXPECT_EQ(new_request.origin()->url(), origin_url);
  EXPECT_EQ(origin.origin(), nullptr);
}

}  // namespace plusfish
