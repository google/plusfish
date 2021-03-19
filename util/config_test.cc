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

#include "util/config.h"

#include "gtest/gtest.h"
#include "proto/http_request.pb.h"
#include "proto/security_check.pb.h"
#include "request.h"

namespace plusfish {
namespace util {

TEST(ConfigUtilTest, ParsesOk) {
  SecurityCheckConfig config;
  ASSERT_TRUE(
      LoadCheckConfigs(absl::GetFlag(FLAGS_test_srcdir) +
                           "/google3/third_party/plusfish/config/*.asciipb",
                       &config));
  // Simple checks to make sure something actually loaded.
  ASSERT_LT(1, config.security_test_size());
  EXPECT_TRUE(config.security_test(0).has_name());
}

TEST(ConfigUtilTest, ParsesNotOk) {
  SecurityCheckConfig config;
  ASSERT_FALSE(
      LoadCheckConfigs(absl::GetFlag(FLAGS_test_srcdir) +
                           "/google3/third_party/plusfish/config/*.nope",
                       &config));
}

TEST(ConfigUtilTest, ParsesRequestsOk) {
  HttpRequestCollection requests;
  ASSERT_TRUE(LoadRequestsConfig(
      FLAGS_test_srcdir +
          "/google3/third_party/plusfish/util/testdata/test_requests.asciipb",
      &requests));
  // Simple checks to make sure something actually loaded.
  ASSERT_EQ(4, requests.request_size());
  for (HttpRequest request : requests.request()) {
    Request req(request);
    ASSERT_EQ(req.url(), request.url());
  }
}

}  // namespace util
}  // namespace plusfish
