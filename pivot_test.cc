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

#include "pivot.h"
#include <string>

#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "testing/reporter_mock.h"

using testing::NotNull;

namespace plusfish {

class PivotTest : public ::testing::Test {
 protected:
  PivotTest() : pivot_("foo") {}
  Pivot pivot_;
};

TEST_F(PivotTest, AddRequestTest) {
  std::string url1 = "http://www.google.com/";
  std::string url2 = "http://www.google.com/foo";

  std::unique_ptr<Request> req1(new Request(url1));
  std::unique_ptr<Request> req2(new Request(url1));
  std::unique_ptr<Request> req3(new Request(url2));

  EXPECT_FALSE(nullptr == pivot_.AddRequest(std::move(req1)));
  // Same URL: should not be added or claimed.
  EXPECT_TRUE(nullptr == pivot_.AddRequest(std::move(req2)));
  // New URL: should be added and claimed.
  EXPECT_FALSE(nullptr == pivot_.AddRequest(std::move(req3)));
}

TEST_F(PivotTest, GetChildrenTest) {
  EXPECT_TRUE(pivot_.AddChildPivot(std::unique_ptr<Pivot>(new Pivot("one"))));
  EXPECT_TRUE(pivot_.AddChildPivot(std::unique_ptr<Pivot>(new Pivot("two"))));

  EXPECT_TRUE(nullptr == pivot_.GetChildPivot("foo"));
  EXPECT_FALSE(nullptr == pivot_.GetChildPivot("one"));
  EXPECT_FALSE(nullptr == pivot_.GetChildPivot("two"));
}

TEST_F(PivotTest, AddChildrenTest) {
  EXPECT_TRUE(pivot_.AddChildPivot(std::unique_ptr<Pivot>(new Pivot("one"))));
  EXPECT_TRUE(pivot_.AddChildPivot(std::unique_ptr<Pivot>(new Pivot("two"))));

  EXPECT_FALSE(pivot_.AddChildPivot(std::unique_ptr<Pivot>(new Pivot("one"))));
  EXPECT_FALSE(pivot_.AddChildPivot(std::unique_ptr<Pivot>(new Pivot("two"))));
}

TEST_F(PivotTest, TestTemplateLimit) {
  std::string url1 = "http://www.google.com/";
  std::string url2 = "http://www.google.com/foo";

  std::unique_ptr<Request> within_limit(new Request(url1));
  std::unique_ptr<Request> over_limit(new Request(url1));

  Pivot test("request_limit_test", 1);
  EXPECT_FALSE(nullptr == test.AddRequest(std::move(within_limit)));
  EXPECT_TRUE(nullptr == test.AddRequest(std::move(over_limit)));
}

TEST_F(PivotTest, TestChildLimit) {
  Pivot test("child_limit_test", 1, 1);
  EXPECT_TRUE(test.AddChildPivot(std::unique_ptr<Pivot>(new Pivot("one"))));
  EXPECT_FALSE(test.AddChildPivot(std::unique_ptr<Pivot>(new Pivot("one"))));
}

TEST_F(PivotTest, TestReportOk) {
  int depth = 1;
  testing::MockReporter mock_reporter;
  Pivot test_pivot("test");
  EXPECT_TRUE(
      test_pivot.AddChildPivot(std::unique_ptr<Pivot>(new Pivot("one"))));
  EXPECT_TRUE(
      test_pivot.AddChildPivot(std::unique_ptr<Pivot>(new Pivot("two"))));
  EXPECT_CALL(mock_reporter, ReportPivot(NotNull(), depth + 1)).Times(2);
  test_pivot.Report(&mock_reporter, depth);
}

}  // namespace plusfish
