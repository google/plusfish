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

#include "audit/util/issue_util.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/container/node_hash_set.h"
#include "proto/issue_details.pb.h"

namespace plusfish {

TEST(GumboUtilTest, UpdateIssueVectorWithSnippet) {
  absl::node_hash_set<std::unique_ptr<IssueDetails>> issues;
  int64 tested_offset = 2;
  util::UpdateIssueVectorWithSnippet(IssueDetails::XSS_REFLECTED_ATTRIBUTE,
                                     Severity::HIGH, 42, "aaaaaa",
                                     tested_offset, "extra_info", &issues);

  EXPECT_EQ(1, issues.size());
  EXPECT_EQ(tested_offset, issues.begin()->get()->response_body_offset());
  EXPECT_STREQ(issues.begin()->get()->response_snippet().c_str(), "aaaaaa");
}

TEST(GumboUtilTest, UpdateIssueVectorWithSmallerSnippet) {
  std::string response_body("0123456789");
  response_body.resize(response_body.length() + 100, 'A');
  response_body.append("0123456789");
  response_body.resize(response_body.length() + 100, 'B');

  absl::node_hash_set<std::unique_ptr<IssueDetails>> issues;
  int64 tested_offset = 110;
  util::UpdateIssueVectorWithSnippet(IssueDetails::XSS_REFLECTED_ATTRIBUTE,
                                     Severity::HIGH, 42, response_body,
                                     tested_offset, "extra_info", &issues);

  EXPECT_EQ(1, issues.size());
  EXPECT_EQ(tested_offset, issues.begin()->get()->response_body_offset());
  EXPECT_EQ(issues.begin()->get()->response_snippet().size(), 200);
}

TEST(GumboUtilTest, UpdateIssueVectorWithSmallerSnippetMissingPrefix) {
  std::string response_body("0123456789");
  response_body.resize(response_body.length() + 100, 'A');

  absl::node_hash_set<std::unique_ptr<IssueDetails>> issues;
  int64 tested_offset = 0;
  util::UpdateIssueVectorWithSnippet(IssueDetails::XSS_REFLECTED_ATTRIBUTE,
                                     Severity::HIGH, 42, response_body,
                                     tested_offset, "extra_info", &issues);

  EXPECT_EQ(1, issues.size());
  EXPECT_EQ(tested_offset, issues.begin()->get()->response_body_offset());
  // The offset is 0 so there is no prefix snippet. Therefore the expected
  // length here is the size of the suffix snippet.
  EXPECT_EQ(issues.begin()->get()->response_snippet().size(), 100);
}

}  // namespace plusfish
