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

#include "parsers/util/escaping.h"
#include <vector>

#include <gtest/gtest.h>
#include "gtest/gtest.h"

namespace plusfish {
namespace util {

TEST(ScrapeUtilTest, ScrapeUnescapeHtml) {
  EXPECT_STREQ("hello&", UnescapeHtml("hello&amp;").c_str());
  EXPECT_STREQ("hello&&", UnescapeHtml("hello&&amp;").c_str());
  EXPECT_STREQ("<>", UnescapeHtml("&lt;&gt;").c_str());
}

TEST(ScrapeUtilTest, ScrapeUnescapeHtmlEmptyString) {
  EXPECT_STREQ("", UnescapeHtml("").c_str());
}

TEST(ScrapeUtilTest, ScrapeEscapeHtmlEmptyString) {
  EXPECT_STREQ("", EscapeHtml("").c_str());
}

TEST(ScrapeUtilTest, ScrapeEscapeHtmlString) {
  EXPECT_STREQ("&lt;&gt;&quot;", EscapeHtml("<>\"").c_str());
}

}  // namespace util
}  // namespace plusfish
