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

#include "report/text_reporter.h"

#include <memory>

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "pivot.h"
#include "request.h"
#include "testing/fake_file_writer.h"

using testing::AllOf;
using testing::HasSubstr;

namespace plusfish {

class TextReporterTest : public ::testing::Test {
 protected:
  TextReporterTest() {
    writer_mock_.reset(new testing::FakeFileWriter(&file_content_));
    reporter_.reset(new TextReporter(std::move(writer_mock_)));
  }

  std::string file_content_;
  std::unique_ptr<testing::FakeFileWriter> writer_mock_;
  std::unique_ptr<TextReporter> reporter_;
};

TEST_F(TextReporterTest, ReportOk) {
  std::string first_url("http://test:80/");
  std::string second_url("http://test:80/me");
  Pivot test_pivot("ignored");
  std::unique_ptr<Request> issue_request(new Request(first_url));

  IssueDetails details;
  details.set_type(IssueDetails_IssueType_BLIND_SQL_INJECTION);
  issue_request->AddIssue(&details);

  test_pivot.AddRequest(std::move(issue_request));
  test_pivot.AddRequest(std::unique_ptr<Request>(new Request(second_url)));

  reporter_->ReportPivot(&test_pivot, 1);
  EXPECT_THAT(file_content_.c_str(),
              AllOf(HasSubstr(first_url), HasSubstr(second_url),
                    HasSubstr("BLIND_SQL_INJECTION")));
}

TEST_F(TextReporterTest, ClosesFileOnDelete) {
  writer_mock_.reset(new testing::FakeFileWriter(&file_content_));
  EXPECT_CALL(*writer_mock_, Close());
  reporter_.reset(new TextReporter(std::move(writer_mock_)));
  reporter_.reset();
}

}  // namespace plusfish
