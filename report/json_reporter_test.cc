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

#include "report/json_reporter.h"

#include <memory>

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "third_party/jsoncpp/json.h"
#include "pivot.h"
#include "proto/issue_details.pb.h"
#include "proto/report.pb.h"
#include "request.h"
#include "testing/fake_file_writer.h"

namespace plusfish {

class JSONReporterTest : public ::testing::Test {
 protected:
  JSONReporterTest() {
    writer_mock_.reset(new testing::FakeFileWriter(&file_content_));
    reporter_.reset(new JSONReporter(std::move(writer_mock_)));
  }

  std::string file_content_;
  std::unique_ptr<testing::FakeFileWriter> writer_mock_;
  std::unique_ptr<JSONReporter> reporter_;
};

TEST_F(JSONReporterTest, ReportOk) {
  // Create the pivot with requests.
  std::string first_url("http://test:85/");
  std::string second_url("http://test:80/me");
  std::string issue_url("http://test:80/too?issue");

  Pivot test_pivot("ignored");
  Request* first_request = new Request(first_url);
  Request* second_request = new Request(second_url);

  IssueDetails issue;
  issue.set_issue_name("unique_check_name");
  first_request->AddIssue(&issue);

  test_pivot.AddRequest(std::unique_ptr<Request>(first_request));
  test_pivot.AddRequest(std::unique_ptr<Request>(second_request));

  // Create a security check config.
  SecurityCheckConfig config_;
  SecurityTest* sec_test = config_.add_security_test();
  sec_test->set_name("security_test_name");

  reporter_->ReportSecurityConfig(config_);
  reporter_->ReportPivot(&test_pivot, 1);

  // Parse the generated JSON and review the results. This ensures the output
  // is proper JSON.
  Json::Reader reader;
  Json::Value root;
  ASSERT_TRUE(reader.parse(file_content_.append("]}").c_str(), root));
  ASSERT_EQ(root["pivots"].size(), 2);
  EXPECT_EQ(root["pivots"][0]["request"]["host"].asString(), "test");
  EXPECT_EQ(root["pivots"][0]["request"]["port"].asInt(), 85);
  ASSERT_EQ(root["pivots"][0]["issue"].size(), 1);
  ASSERT_EQ(root["pivots"][0]["issue"][0]["issueName"].asString(),
            issue.issue_name());
  EXPECT_EQ(root["config"]["securityTest"][0]["name"].asString(),
            sec_test->name());
}

TEST_F(JSONReporterTest, ClosesFileOnDeleteAndWritesJsonSuffix) {
  writer_mock_.reset(new testing::FakeFileWriter(&file_content_));
  EXPECT_CALL(*writer_mock_, Close());
  reporter_.reset(new JSONReporter(std::move(writer_mock_)));
  EXPECT_EQ(file_content_, "]}");
}

}  // namespace plusfish
