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

#include "parsers/html_scraper.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "request.h"
#include "response.h"
#include "testing/request_mock.h"

using testing::Return;

namespace plusfish {

// All logic is in other classes so this set of tests is simple on purpose.
class HtmlScraperTest : public ::testing::Test {
 protected:
  HtmlScraper scraper_;
};

TEST_F(HtmlScraperTest, ScrapeTextAndElementsOK) {
  std::string url_in_href("http://example.com");
  std::string url_in_text("http://example.com/text");
  Request test_request(url_in_href);
  ASSERT_TRUE(scraper_.Parse(
      &test_request, "<a href='" + url_in_href + "'>" + url_in_text + "</a>"));
  ASSERT_EQ(2, scraper_.anchors().size());
  ASSERT_NE(nullptr, scraper_.fingerprint());
  EXPECT_EQ(url_in_href, scraper_.anchors()[0]);
  EXPECT_EQ(url_in_text, scraper_.anchors()[1]);
}

TEST_F(HtmlScraperTest, ScrapesFormOk) {
  std::string form_url("http://example.com");
  std::string input_field_name("field_name");
  testing::MockRequest test_request(form_url);
  std::string content("<form action='" + form_url + "'>" +
                      "<input type='text' name=" + input_field_name + ">");

  Response response;
  response.set_body(content);
  EXPECT_CALL(test_request, response()).WillOnce(Return(&response));
  ASSERT_TRUE(scraper_.Parse(
      &test_request, "<form action='" + form_url + "'>" +
                         "<input type='text' name=" + input_field_name + ">"));
  ASSERT_EQ(1, scraper_.requests().size());
}

}  // namespace plusfish
