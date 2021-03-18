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

#include "parsers/util/scrape_util.h"
#include <vector>

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "absl/container/node_hash_set.h"
#include "proto/http_common.pb.h"
#include "proto/issue_details.pb.h"

namespace plusfish {
namespace util {

class ScrapeUtilTest : public ::testing::Test {
 public:
  ScrapeUtilTest() {}

 protected:
  // Helper method for parsing JS.
  bool ScrapeJsHelper(const std::string& content) {
    return ScrapeJs(content, &anchors_, &issues_);
  }
  absl::node_hash_set<std::string> anchors_;
  absl::node_hash_set<std::unique_ptr<IssueDetails>> issues_;
};

TEST_F(ScrapeUtilTest, XsrfDoesNotMatchCommonFormStringTypes) {
  EXPECT_FALSE(IsPotentialXSRFToken("Woody Wood"));
  EXPECT_FALSE(IsPotentialXSRFToken("woody@example.org"));
  EXPECT_FALSE(IsPotentialXSRFToken("http://example.org"));
  EXPECT_FALSE(IsPotentialXSRFToken("+1650-253-0000"));
  EXPECT_FALSE(IsPotentialXSRFToken("1600 Amphitheatre Pkwy"));
  EXPECT_FALSE(IsPotentialXSRFToken(""));
}

TEST_F(ScrapeUtilTest, XsrfMatchesHashes) {
  EXPECT_TRUE(IsPotentialXSRFToken("d41d8cd98f00b204e9800998ecf8427e"));
  EXPECT_TRUE(IsPotentialXSRFToken("da39a3ee5e6b4b0d3255bfef95601890afd80709"));
}

TEST_F(ScrapeUtilTest, XsrfMatchesBase64) {
  EXPECT_TRUE(IsPotentialXSRFToken("aGVsbG93b3JsZA=="));
  EXPECT_TRUE(IsPotentialXSRFToken("aGVsbG8hdGhpc2lzZG9nZ3lkb2dib25l"));
  EXPECT_TRUE(
      IsPotentialXSRFToken("G1H1RMvZaWeprHlFCV9DXXy374Q3j3ubNG9P1NKJLjM="));
}

TEST_F(ScrapeUtilTest, XsrfIgnoresLargeBase64Strings) {
  EXPECT_FALSE(IsPotentialXSRFToken(
      "aGVsbG8hdGhpc2lzZG9nZ3lkb2dib25laGVsbG8hdGhpc2lzZG9nZ3lkb2dib25laGVsbG8h"
      "dGhpc2lzZG9nZ3lkb2dib25laGVsbG8hdGhpc2lzZG9nZ3lkb2dib25l"));
}

TEST_F(ScrapeUtilTest, XsrfHandlesLargeBase64SplitStringOk) {
  EXPECT_TRUE(IsPotentialXSRFToken(
      "aGVsbG8hdGhpc2lzZG9nZ3lkb2dib25l:2lzZG9nZ3lkb2dib25laGVsbG8"
      "hdGhpc2lzZG9nZ3lkb2dib25laGVsbG8hdGhpc2lzZG9nZ3lkb2dib25l"));
}

TEST_F(ScrapeUtilTest, SuffixComparisonOk) {
  std::string matches_href("foo.href");
  std::string matches_window_open("window.open");
  std::string matches_none("foo");
  std::string matches_empty("");
  EXPECT_TRUE(JsKeywordSuffixMatch(matches_href));
  EXPECT_TRUE(JsKeywordSuffixMatch(matches_window_open));
  EXPECT_FALSE(JsKeywordSuffixMatch(matches_none));
  EXPECT_FALSE(JsKeywordSuffixMatch(matches_empty));
}

TEST_F(ScrapeUtilTest, ScrapeJsOk) {
  ASSERT_TRUE(
      ScrapeJsHelper("location.href = 'http://google.com'; foo('bar');"));
  ASSERT_EQ(1, anchors_.size());
  ASSERT_STREQ("http://google.com", anchors_.begin()->c_str());
}

TEST_F(ScrapeUtilTest, ScrapeJsNoEndingQuote) {
  ASSERT_TRUE(ScrapeJsHelper("location.href = 'http://google.com"));
  ASSERT_EQ(1, anchors_.size());
  ASSERT_STREQ("http://google.com", anchors_.begin()->c_str());
}

TEST_F(ScrapeUtilTest, ScrapeJsEmptyQuotesProperlyIgnored) {
  ASSERT_TRUE(ScrapeJsHelper("location.href = '';"));
  ASSERT_EQ(0, anchors_.size());
}

TEST_F(ScrapeUtilTest, ScrapeJsOnlyScrapesFirstArgOk) {
  ASSERT_TRUE(ScrapeJsHelper("window.open('/bar', 'this_one_is_ignored');"));
  ASSERT_EQ(1, anchors_.size());
  ASSERT_STREQ("/bar", anchors_.begin()->c_str());
}

TEST_F(ScrapeUtilTest, ScrapeJsOnlyScrapeMultipleOk) {
  ASSERT_TRUE(
      ScrapeJsHelper("document.location = '/hel'; location.href = \"/lo\";"));
  EXPECT_EQ(2, anchors_.size());
  EXPECT_THAT(anchors_, testing::Contains("/hel"));
  EXPECT_THAT(anchors_, testing::Contains("/lo"));
}

TEST_F(ScrapeUtilTest, ScrapeJsTakesEmptyStringOk) {
  ASSERT_TRUE(ScrapeJsHelper(""));
  ASSERT_EQ(anchors_.size(), 0);
}

TEST_F(ScrapeUtilTest, ScrapeJsProperlyIgnoredEscapedQuotes) {
  ASSERT_TRUE(ScrapeJsHelper("document.location = '/he\\'llo';"));
  ASSERT_EQ(1, anchors_.size());
  ASSERT_STREQ("/he\\'llo", anchors_.begin()->c_str());
}

TEST_F(ScrapeUtilTest, ScrapeJsUnbalancedQuotes) {
  ASSERT_TRUE(ScrapeJsHelper("document.location = '/he\"llo';"));
  ASSERT_EQ(1, anchors_.size());
  ASSERT_STREQ("/he\"llo", anchors_.begin()->c_str());
}

TEST_F(ScrapeUtilTest, ScrapeJsFindsQuotedRelativeUrl) {
  ASSERT_TRUE(
      ScrapeJsHelper("never = '/gonna'; give = '../you'; up = '/../never'; "));
  EXPECT_EQ(3, anchors_.size());
  EXPECT_THAT(anchors_, testing::Contains("/gonna"));
  EXPECT_THAT(anchors_, testing::Contains("../you"));
  EXPECT_THAT(anchors_, testing::Contains("/../never"));
}

TEST_F(ScrapeUtilTest, ScrapeJsFindsQuotedAbsoluteUrl) {
  std::string url_tested("http://gonna/let/you/down");
  ASSERT_TRUE(ScrapeJsHelper("never = '" + url_tested + "';"));
  EXPECT_EQ(1, anchors_.size());
  EXPECT_THAT(anchors_, testing::Contains(url_tested));
}

TEST_F(ScrapeUtilTest, ScrapeJsFindsXss) {
  EXPECT_TRUE(ScrapeJsHelper("something=''; plus1234fish;"));
  EXPECT_TRUE(ScrapeJsHelper("something='';plus1234fish;"));
  EXPECT_TRUE(ScrapeJsHelper("plus1234fish;"));
  ASSERT_EQ(3, issues_.size());
  EXPECT_THAT(issues_.begin()->get()->extra_info(),
              testing::HasSubstr("javascript"));
}

TEST_F(ScrapeUtilTest, ScrapeUrlOk) {
  std::string basic_url("http://example.com");
  std::string url_without_scheme("//example.com/aa");
  std::string url_with_params("https://example.com/foo?a=a");

  // The three URLs.
  anchors_ =
      ScrapeUrl(basic_url + " " + url_without_scheme + " " + url_with_params);

  EXPECT_EQ(3, anchors_.size());
  EXPECT_THAT(anchors_, testing::Contains(url_without_scheme));
  EXPECT_THAT(anchors_, testing::Contains(url_with_params));
  EXPECT_THAT(anchors_, testing::Contains(basic_url));
}

TEST_F(ScrapeUtilTest, SniffHtmlMime) {
  MimeInfo::MimeType orig_mime = MimeInfo::EXT_PDF;
  std::string content_to_sniff("choo choo <html>");
  EXPECT_TRUE(SniffMimeType(content_to_sniff, &orig_mime));
  EXPECT_EQ(orig_mime, MimeInfo::ASC_HTML);
}

TEST_F(ScrapeUtilTest, SniffHtmlMimeBigString) {
  MimeInfo::MimeType orig_mime = MimeInfo::EXT_PDF;
  std::string content_to_sniff("choo choo <html>");
  content_to_sniff.resize(6000, 'A');
  EXPECT_TRUE(SniffMimeType(content_to_sniff, &orig_mime));
  EXPECT_EQ(orig_mime, MimeInfo::ASC_HTML);
}

TEST_F(ScrapeUtilTest, SniffHtmlMimeCase) {
  MimeInfo::MimeType orig_mime = MimeInfo::EXT_PDF;
  std::string content_to_sniff("<P>choo choo</P>");
  EXPECT_TRUE(SniffMimeType(content_to_sniff, &orig_mime));
  EXPECT_EQ(orig_mime, MimeInfo::ASC_HTML);
}

TEST_F(ScrapeUtilTest, SniffHtmlMimeUnknown) {
  MimeInfo::MimeType orig_mime = MimeInfo::EXT_PDF;
  std::string content_to_sniff("<choo></choo>");
  EXPECT_FALSE(SniffMimeType(content_to_sniff, &orig_mime));
  EXPECT_EQ(orig_mime, MimeInfo::EXT_PDF);
}

TEST_F(ScrapeUtilTest, SniffXmlGeneric) {
  MimeInfo::MimeType orig_mime = MimeInfo::EXT_PDF;
  std::string content_to_sniff("<?xml version=1 encoding=\"utf-8\"?>");
  EXPECT_TRUE(SniffMimeType(content_to_sniff, &orig_mime));
  EXPECT_EQ(orig_mime, MimeInfo::XML_GENERIC);
}

TEST_F(ScrapeUtilTest, SniffXmlHtml) {
  MimeInfo::MimeType orig_mime = MimeInfo::EXT_PDF;
  std::string content_to_sniff(
      "<?xml version=1 encoding=\"utf-8\"?>"
      "<!doctype html></html>");
  EXPECT_TRUE(SniffMimeType(content_to_sniff, &orig_mime));
  EXPECT_EQ(orig_mime, MimeInfo::XML_XHTML);
}

}  // namespace util
}  // namespace plusfish
