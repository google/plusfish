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

#include "audit/generic_generator.h"

#include <memory>

#include "file/base/fileutils.h"
#include "file/base/helpers.h"
#include "file/base/path.h"
#include "gtest/gtest.h"
#include "proto/generator.pb.h"
#include "request.h"

namespace plusfish {
class GenericGeneratorTest : public ::testing::Test {
 protected:
  GenericGeneratorTest() {}

  void GeneratorFromFile(const std::string& file) {
    CHECK_OK(file::GetTextProto(
        file::JoinPath(absl::GetFlag(FLAGS_test_srcdir),
                       "google3/third_party/plusfish/audit/testdata/", file),
        &rule_, file::Defaults()));
    generator_.reset(new GenericGenerator(rule_));
  }

  std::unique_ptr<GenericGenerator> generator_;
  std::vector<std::unique_ptr<Request>> requests_;
  GeneratorRule rule_;
};

TEST_F(GenericGeneratorTest, NoParametersNoRequests) {
  GeneratorFromFile("base_generator_rule.asciipb");
  Request base_request("http://www.example.com:80/");
  EXPECT_EQ(0, generator_->Generate(&base_request, &requests_));
  EXPECT_EQ(0, requests_.size());
}

TEST_F(GenericGeneratorTest, SingleParameterInjectionWithoutEncoding) {
  GeneratorFromFile("base_generator_rule.asciipb");
  Request base_request("http://www.example.com:80/?file=bar");
  ASSERT_EQ(2, generator_->Generate(&base_request, &requests_));
  ASSERT_EQ(2, requests_.size());
  EXPECT_STREQ(requests_[0]->url().c_str(),
               "http://www.example.com:80/?file=../../../etc/passwd");
  EXPECT_STREQ(requests_[1]->url().c_str(),
               "http://www.example.com:80/?file=/etc/passwd");
}

TEST_F(GenericGeneratorTest, GeneratedRequestsHaveParentId) {
  GeneratorFromFile("base_generator_rule.asciipb");
  Request base_request("http://www.example.com:80/?file=bar");
  base_request.set_id(42);
  ASSERT_EQ(2, generator_->Generate(&base_request, &requests_));
  ASSERT_EQ(2, requests_.size());
  EXPECT_EQ(requests_[0]->parent_id(), 42);
  EXPECT_EQ(requests_[1]->parent_id(), 42);
}

TEST_F(GenericGeneratorTest, SingleParameterWithURLEncoding) {
  GeneratorFromFile("generator_rule_url_encode.asciipb");
  Request base_request("http://www.example.com:80/?file=bar");
  ASSERT_EQ(2, generator_->Generate(&base_request, &requests_));
  ASSERT_EQ(2, requests_.size());
  EXPECT_STREQ(requests_[0]->url().c_str(),
               "http://www.example.com:80/?file=%2e%2e%2fetc%2fpasswd");
  EXPECT_STREQ(requests_[1]->url().c_str(),
               "http://www.example.com:80/?file=%2fetc%2fpasswd");
}

TEST_F(GenericGeneratorTest, SingleParameterInjectionMultipleInjectionMethods) {
  GeneratorFromFile("generator_rule_append_replace.asciipb");
  Request base_request("http://www.example.com:80/?file=foo");
  ASSERT_EQ(4, generator_->Generate(&base_request, &requests_));
  EXPECT_STREQ(requests_[0]->url().c_str(),
               "http://www.example.com:80/?file=../../../etc/passwd");
  EXPECT_STREQ(requests_[3]->url().c_str(),
               "http://www.example.com:80/?file=foo/etc/passwd");
}

TEST_F(GenericGeneratorTest, MultipleParametersButOnlyOneChosen) {
  GeneratorFromFile("generator_rule_single_param.asciipb");
  Request base_request("http://www.example.com:80/?foo=bar&fake=cake");
  ASSERT_EQ(2, generator_->Generate(&base_request, &requests_));
  EXPECT_STREQ(requests_[0]->url().c_str(),
               "http://www.example.com:80/?foo=../../../etc/passwd&fake=cake");
  EXPECT_STREQ(requests_[1]->url().c_str(),
               "http://www.example.com:80/?foo=/etc/passwd&fake=cake");
}

TEST_F(GenericGeneratorTest, InjectInSingleHeader) {
  GeneratorFromFile("generator_rule_single_header.asciipb");
  Request base_request("http://www.example.com:80/");
  ASSERT_EQ(1, generator_->Generate(&base_request, &requests_));
  ASSERT_EQ(1, requests_[0]->proto().header_size());
  ASSERT_STREQ(requests_[0]->proto().header(0).name().c_str(), "User-Agent");
  ASSERT_STREQ(requests_[0]->proto().header(0).value().c_str(),
               "payload_value");
}

TEST_F(GenericGeneratorTest, MultipleParameterInjection) {
  GeneratorFromFile("base_generator_rule.asciipb");
  Request base_request("http://www.example.com:80/?one=foo&two=bar");
  ASSERT_EQ(4, generator_->Generate(&base_request, &requests_));
  EXPECT_STREQ(requests_[0]->url().c_str(),
               "http://www.example.com:80/?one=../../../etc/passwd&two=bar");
  EXPECT_STREQ(requests_[1]->url().c_str(),
               "http://www.example.com:80/?one=/etc/passwd&two=bar");
  EXPECT_STREQ(requests_[2]->url().c_str(),
               "http://www.example.com:80/?one=foo&two=../../../etc/passwd");
  EXPECT_STREQ(requests_[3]->url().c_str(),
               "http://www.example.com:80/?one=foo&two=/etc/passwd");
}

TEST_F(GenericGeneratorTest, BodyParameterInjection) {
  GeneratorFromFile("generator_rule_body_param.asciipb");
  Request base_request("http://www.example.com:80");
  base_request.SetPostParameter("this", "is_dog", false /* replace existing*/);
  ASSERT_EQ(2, generator_->Generate(&base_request, &requests_));
  EXPECT_STREQ(requests_[0]->GetRequestBody().c_str(),
               "this=../../../etc/passwd");
  EXPECT_STREQ(requests_[1]->GetRequestBody().c_str(), "this=/etc/passwd");
}

TEST_F(GenericGeneratorTest, PathSegmentInjection) {
  GeneratorFromFile("generator_rule_path_elements.asciipb");
  Request base_request("http://www.example.com:80/path/here");
  EXPECT_EQ(2, generator_->Generate(&base_request, &requests_));
  EXPECT_STREQ(requests_[0]->url().c_str(),
               "http://www.example.com:80/path<XSS>/here");
  EXPECT_STREQ(requests_[1]->url().c_str(),
               "http://www.example.com:80/path/here<XSS>");
}

TEST_F(GenericGeneratorTest, LastPathSegmentInjection) {
  GeneratorFromFile("generator_rule_last_path_elements.asciipb");
  Request base_request("http://www.example.com:80/here.jsp");
  EXPECT_EQ(1, generator_->Generate(&base_request, &requests_));
  EXPECT_STREQ(requests_[0]->url().c_str(),
               "http://www.example.com:80/<XSS>here.jsp");
}

TEST_F(GenericGeneratorTest, InjectedFieldIsMarkedModified) {
  GeneratorFromFile("generator_rule_last_path_elements.asciipb");
  Request base_request("http://www.example.com:80/here.jsp");
  EXPECT_EQ(1, generator_->Generate(&base_request, &requests_));
  EXPECT_TRUE(requests_[0]->proto().path(0).modified());
}

TEST_F(GenericGeneratorTest, InjectInMultipleTargets) {
  GeneratorFromFile("generator_rule_multiple_targets.asciipb");
  Request base_request("http://www.example.com:80/hello?who=this");
  base_request.SetPostParameter("this", "is_dog", false /* replace existing*/);
  ASSERT_EQ(4, generator_->Generate(&base_request, &requests_));
  EXPECT_STREQ(requests_[0]->url().c_str(),
               "http://www.example.com:80/hello?who=../../../etc/passwd");
  EXPECT_STREQ(requests_[1]->url().c_str(),
               "http://www.example.com:80/hello?who=/etc/passwd");
  EXPECT_STREQ(requests_[2]->GetRequestBody().c_str(),
               "this=../../../etc/passwd");
  EXPECT_STREQ(requests_[3]->GetRequestBody().c_str(), "this=/etc/passwd");

  // Ensure that the parameter of the previous request is not set to modified
  // anymore.
  EXPECT_TRUE(requests_[2]->proto().body_param(0).modified());
  EXPECT_FALSE(requests_[2]->proto().param(0).modified());
}
}  // namespace plusfish
