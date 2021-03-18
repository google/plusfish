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

#include "parsers/gumbo_xss_filter.h"

#include <memory>

#include "strings/case.h"
#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "absl/container/node_hash_set.h"
#include "absl/strings/ascii.h"
#include <gumbo.h>
#include "parsers/gumbo_filter.h"
#include "parsers/gumbo_parser.h"
#include "proto/issue_details.pb.h"
#include "response.h"
#include "testing/request_mock.h"
#include "util/gtl/map_util.h"

using ::testing::HasSubstr;
using ::testing::Return;

namespace plusfish {

class GumboXssFilterTest : public ::testing::Test {
 protected:
  GumboXssFilterTest() {
    request_.reset(new testing::MockRequest("http://foo"));
    request_->set_id(42);
  }
  void SetUp() override {
    xss_filter_.reset(new GumboXssFilter(*request_, &issues_));
    filters_.emplace_back(xss_filter_.get());
    gumbo_.reset(new GumboParser());
  }

  void Parse(const std::string& content) {
    response_.reset(new Response());
    response_->set_body(content);
    gumbo_->Parse(content);
  }

  std::string payload_;
  absl::node_hash_set<std::unique_ptr<IssueDetails>> issues_;
  std::unique_ptr<GumboParser> gumbo_;
  std::unique_ptr<GumboXssFilter> xss_filter_;
  std::unique_ptr<testing::MockRequest> request_;
  std::unique_ptr<Response> response_;
  std::vector<GumboFilter*> filters_;
};

TEST_F(GumboXssFilterTest, FindXssTagOk) {
  std::string buffer = "sdddas <plus42fish>";
  Parse(buffer);
  EXPECT_CALL(*request_, response()).WillOnce(Return(response_.get()));
  gumbo_->FilterDocument(filters_);
  ASSERT_EQ(1, issues_.size());
  EXPECT_EQ(issues_.begin()->get()->severity(), Severity::HIGH);
  EXPECT_EQ(issues_.begin()->get()->request_id(), request_->id());
  EXPECT_THAT(issues_.begin()->get()->extra_info(), HasSubstr("payload"));
  EXPECT_EQ(IssueDetails::XSS_REFLECTED_TAG, issues_.begin()->get()->type());
}

TEST_F(GumboXssFilterTest, DoesntFindXssOk) {
  std::string buffer = "sdddas <foo nothing='plus42fish'>";
  Parse(buffer);
  gumbo_->FilterDocument(filters_);
  EXPECT_EQ(0, issues_.size());
}

TEST_F(GumboXssFilterTest, FindXssAttributeInNormalElement) {
  std::string buffer = "<p plus42fish='1'>";
  Parse(buffer);
  EXPECT_CALL(*request_, response()).WillOnce(Return(response_.get()));
  gumbo_->FilterDocument(filters_);
  ASSERT_EQ(1, issues_.size());
  EXPECT_EQ(issues_.begin()->get()->severity(), Severity::HIGH);
  EXPECT_EQ(issues_.begin()->get()->request_id(), request_->id());
  EXPECT_EQ(IssueDetails::XSS_REFLECTED_ATTRIBUTE,
            issues_.begin()->get()->type());
}

TEST_F(GumboXssFilterTest, FindXssEmptyAttributeInNormalElement) {
  std::string buffer = "<p plus42fish>";
  Parse(buffer);
  EXPECT_CALL(*request_, response()).WillOnce(Return(response_.get()));
  gumbo_->FilterDocument(filters_);
  ASSERT_EQ(1, issues_.size());
  EXPECT_EQ(issues_.begin()->get()->severity(), Severity::HIGH);
  EXPECT_EQ(issues_.begin()->get()->request_id(), request_->id());
  EXPECT_EQ(IssueDetails::XSS_REFLECTED_ATTRIBUTE,
            issues_.begin()->get()->type());
}

TEST_F(GumboXssFilterTest, FindCaseUpperCaseXssTagOk) {
  std::string buffer = "<A PLUS42FISH>";
  Parse(buffer);
  EXPECT_CALL(*request_, response()).WillOnce(Return(response_.get()));
  gumbo_->FilterDocument(filters_);
  ASSERT_EQ(1, issues_.size());
  EXPECT_EQ(issues_.begin()->get()->severity(), Severity::HIGH);
  EXPECT_EQ(issues_.begin()->get()->request_id(), request_->id());
  EXPECT_THAT(issues_.begin()->get()->extra_info(), HasSubstr("payload"));
  EXPECT_EQ(IssueDetails::XSS_REFLECTED_ATTRIBUTE,
            issues_.begin()->get()->type());
}

TEST_F(GumboXssFilterTest, FindsNoXssWhenNoneInjected) {
  std::string buffer = "<p>";
  Parse(buffer);
  gumbo_->FilterDocument(filters_);
  ASSERT_EQ(0, issues_.size());
}

}  // namespace plusfish
