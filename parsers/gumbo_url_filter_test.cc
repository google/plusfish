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

#include "parsers/gumbo_url_filter.h"

#include <memory>

#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "absl/container/node_hash_set.h"
#include <gumbo.h>
#include "parsers/gumbo_filter.h"
#include "parsers/gumbo_parser.h"
#include "proto/issue_details.pb.h"

namespace plusfish {

class GumboUrlFilterTest : public ::testing::Test {
 protected:
  GumboUrlFilterTest() {}

  void SetUp() override {
    url_filter_.reset(new GumboUrlFilter(&anchors_, &issues_));
    filters_.emplace_back(url_filter_.get());
    gumbo_.reset(new GumboParser());
  }

  void ParseAndFilter(const std::string& content_to_parse) {
    gumbo_->Parse(content_to_parse);
    gumbo_->FilterDocument(filters_);
  }

  std::vector<std::string> anchors_;
  absl::node_hash_set<std::unique_ptr<IssueDetails>> issues_;
  std::vector<GumboFilter*> filters_;
  std::unique_ptr<GumboParser> gumbo_;
  std::unique_ptr<GumboUrlFilter> url_filter_;
};

TEST_F(GumboUrlFilterTest, ParseUrlFromKnownAttribute) {
  ParseAndFilter("<a href='hello'>hello</a>");
  ASSERT_EQ(1, anchors_.size());
  EXPECT_STREQ("hello", anchors_[0].c_str());
  EXPECT_STREQ("hello", anchors_[0].c_str());
}

TEST_F(GumboUrlFilterTest, ParseUrlFromRandomAttribute) {
  ParseAndFilter("<a foo='//hello'>hello</a>");
  ASSERT_EQ(1, anchors_.size());
  EXPECT_STREQ("//hello", anchors_[0].c_str());
}

TEST_F(GumboUrlFilterTest, ParseContentFromRandomAttributeIgnored) {
  ParseAndFilter("<a foo='hello'>hello</a>");
  ASSERT_EQ(0, anchors_.size());
}

TEST_F(GumboUrlFilterTest, ParseSkipDuplicateUrls) {
  ParseAndFilter("<a href='hello'></a><a href='hello'></a>");
  ASSERT_EQ(1, anchors_.size());
  EXPECT_STREQ("hello", anchors_[0].c_str());
}

TEST_F(GumboUrlFilterTest, ParseUrlFromTextElement) {
  std::string basic_url("http://example.com");
  ParseAndFilter("<b>" + basic_url + "</b>");
  ASSERT_EQ(1, anchors_.size());
  EXPECT_EQ(basic_url, anchors_[0]);
}

TEST_F(GumboUrlFilterTest, ParseMultipleUrlsFromTextElements) {
  std::string basic_url("http://example.com");
  std::string url_without_scheme("//example.com/aa");
  std::string url_with_params("https://example.com/foo?a=a");

  // The three URLs are spread over two text elements.
  ParseAndFilter("<b>" + basic_url + "</b>" + url_without_scheme + " foobar " +
                 url_with_params);

  ASSERT_EQ(3, anchors_.size());
  ASSERT_EQ(1, std::count(anchors_.begin(), anchors_.end(), basic_url));
  ASSERT_EQ(1,
            std::count(anchors_.begin(), anchors_.end(), url_without_scheme));
  ASSERT_EQ(1, std::count(anchors_.begin(), anchors_.end(), url_with_params));
}

TEST_F(GumboUrlFilterTest, ParseFromJavascriptUrl) {
  ParseAndFilter("<a href=\"javascript:location = '/here';\"></a>");
  ASSERT_EQ(1, anchors_.size());
  EXPECT_STREQ("/here", anchors_[0].c_str());
}

TEST_F(GumboUrlFilterTest, ParseFromVbscriptUrl) {
  ParseAndFilter("<a href=\"vbscript:location.href = '/here';\"></a>");
  ASSERT_EQ(1, anchors_.size());
  EXPECT_STREQ("/here", anchors_[0].c_str());
}

TEST_F(GumboUrlFilterTest, ParseFromJavascriptMixedCaseUrl) {
  ParseAndFilter("<a href=\"jaVascript:location = '/again';\"></a>");
  ASSERT_EQ(1, anchors_.size());
  EXPECT_STREQ("/again", anchors_[0].c_str());
}

TEST_F(GumboUrlFilterTest, ParseUrlEmptyJavascriptUrl) {
  ParseAndFilter("<a href=\"javascript:\"></a>");
  ASSERT_EQ(0, anchors_.size());
}

TEST_F(GumboUrlFilterTest, ParseUrlFromMetaTag) {
  std::string url("http://meta");
  ParseAndFilter("<meta http-equiv=\"refresh\" content=\"5; url=" + url +
                 "\"/>");
  ASSERT_EQ(1, anchors_.size());
  EXPECT_STREQ("http://meta", anchors_[0].c_str());
}

TEST_F(GumboUrlFilterTest, ParseUrlFromMetaTagIgnoresCase) {
  std::string url("http://meta");
  ParseAndFilter("<meta http-equiv=\"refresh\" content=\"5; uRl=" + url +
                 "\"/>");
  ASSERT_EQ(1, anchors_.size());
  EXPECT_STREQ("http://meta", anchors_[0].c_str());
}

TEST_F(GumboUrlFilterTest, ParseQuotedUrlFromMetaTag) {
  std::string url("http://meta");
  ParseAndFilter("<meta http-equiv=\"refresh\" content=\"5; url='" + url +
                 "'\"/>");
  ASSERT_EQ(1, anchors_.size());
  EXPECT_STREQ("http://meta", anchors_[0].c_str());
}

TEST_F(GumboUrlFilterTest, ParseUrlFromMetaTagWithOneQuote) {
  std::string url("http://meta");
  ParseAndFilter("<meta http-equiv=\"refresh\" content=\"5; url='" + url +
                 "\"/>");
  ASSERT_EQ(1, anchors_.size());
  EXPECT_STREQ("http://meta", anchors_[0].c_str());
}

TEST_F(GumboUrlFilterTest, ParseUrlFromMetaWithoutContent) {
  ParseAndFilter("<meta http-equiv=\"\"/>");
  ASSERT_EQ(0, anchors_.size());
}

TEST_F(GumboUrlFilterTest, ParsedGumboUrlIsUnescaped) {
  ParseAndFilter("<a href='hello&amp;'>hello</a>");
  ASSERT_EQ(1, anchors_.size());
  EXPECT_STREQ("hello&", anchors_[0].c_str());
}

TEST_F(GumboUrlFilterTest, ParseUrlOnclick) {
  ParseAndFilter("<p onclick=\"location.href='/url';\">");
  ASSERT_EQ(1, anchors_.size());
}

TEST_F(GumboUrlFilterTest, ParseUrlOnclickFindsXss) {
  ParseAndFilter(
      "<p onclick=\"var customerName='';plus123fish;'; "
      "alert('Welcome Mr. ' + customerName);\">");
  ASSERT_EQ(1, issues_.size());
}

}  // namespace plusfish
