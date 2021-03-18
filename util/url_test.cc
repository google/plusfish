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

#include "util/url.h"

#include <string>

#include "gtest/gtest.h"

namespace plusfish {
namespace util {

TEST(UrlTest, StripUrlWithSuffixOk) {
  std::string url = "http://test/foo";
  std::string result = StripUrlFileSuffix(url);
  EXPECT_EQ(result, "http://test/");
}

TEST(UrlTest, StripUrlWithoutSlashOk) {
  std::string url = "http://test";
  std::string result = StripUrlFileSuffix(url);
  EXPECT_EQ(result, "http://test/");
}

TEST(UrlTest, StripUrlWithLongPathOk) {
  std::string expected_url = "http://test/aa/bb/cc/dd/ee/ff/gg/";
  std::string result = StripUrlFileSuffix(expected_url + "strip_me");
  EXPECT_EQ(result, expected_url);
}

TEST(UrlTest, StripUrlIgnoresSlashInParameter) {
  std::string expected_url = "http://test/aa?bb=cc/dd/ee/ff/gg/";
  std::string result = StripUrlFileSuffix(expected_url + "strip_me");
  EXPECT_EQ(result, "http://test/");
}

}  // namespace util
}  // namespace plusfish
