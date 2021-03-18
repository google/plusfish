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

#include "datastore.h"

#include <vector>

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "proto/http_response.pb.h"
#include "proto/issue_details.pb.h"
#include "report/reporter.h"
#include "request.h"
#include "response.h"
#include "testing/reporter_mock.h"
#include "testing/request_mock.h"
#include "util/html_fingerprint.h"

ABSL_DECLARE_FLAG(int32_t, max_issues_per_url);
ABSL_DECLARE_FLAG(int32_t, max_issues_per_check_per_url);
ABSL_DECLARE_FLAG(int32_t, max_issues_per_check);
ABSL_DECLARE_FLAG(int32_t, pivot_child_limit);

using testing::NotNull;
using testing::Property;
using testing::StrEq;

namespace plusfish {

class DataStoreTest : public ::testing::Test {
 public:
  DataStoreTest() {}

  void SetUp() override {
    datastore_.reset(new DataStore());
    datastore_->AddHost("example.org");
    mock_request_.reset(new testing::MockRequest("http://example.org"));
  }
  std::unique_ptr<DataStore> datastore_;
  std::unique_ptr<testing::MockRequest> mock_request_;
};

TEST_F(DataStoreTest, AddDifferentRequestsOk) {
  std::unique_ptr<Request> request_one(
      new Request("http://example.org/see/you"));
  std::unique_ptr<Request> request_two(
      new Request("http://example.org/you/see"));

  int64 id_one = datastore_->AddRequest(std::move(request_one));
  int64 id_two = datastore_->AddRequest(std::move(request_two));

  ASSERT_THAT(datastore_->GetRequestById(id_one), NotNull());
  ASSERT_THAT(datastore_->GetRequestById(id_two), NotNull());
  EXPECT_EQ(id_one, datastore_->GetRequestById(id_one)->id());
  EXPECT_EQ(id_two, datastore_->GetRequestById(id_two)->id());
  EXPECT_EQ(2, datastore_->probe_queue_size());
}

TEST_F(DataStoreTest, GetRequestByIdFail) {
  EXPECT_EQ(nullptr, datastore_->GetRequestById(42));
}

TEST_F(DataStoreTest, PivotTreeCreatedOk) {
  std::unique_ptr<Request> request_one(new Request("http://example.org/one/"));
  std::unique_ptr<Request> request_two(
      new Request("http://example.org/one/two/three"));

  EXPECT_GT(datastore_->AddRequest(std::move(request_one)),
            DataStore::kInvalidId);
  EXPECT_GT(datastore_->AddRequest(std::move(request_two)),
            DataStore::kInvalidId);

  const auto& pivot_map = datastore_->site_pivots();
  EXPECT_EQ(1, pivot_map.count("example.org"));
  Pivot* site_pivot = pivot_map.find("example.org")->second.get();

  // Verify the child pivots.
  Pivot* child_pivot = site_pivot->GetChildPivot("one");
  EXPECT_FALSE(nullptr == child_pivot);
  child_pivot = child_pivot->GetChildPivot("two");
  EXPECT_FALSE(nullptr == child_pivot);
  child_pivot = child_pivot->GetChildPivot("three");
  EXPECT_FALSE(nullptr == child_pivot);
  EXPECT_EQ(2, datastore_->probe_queue_size());
}

TEST_F(DataStoreTest, RequestWithOutOfScopeHostIsRejected) {
  std::unique_ptr<Request> request(
      new Request("http://example.com/oh/no/you/dont"));
  EXPECT_EQ(datastore_->AddRequest(std::move(request)), DataStore::kInvalidId);
}

TEST_F(DataStoreTest, RequestWithInvalidUrlIsRejected) {
  std::unique_ptr<Request> request(new Request("will_not_fly"));
  EXPECT_EQ(datastore_->AddRequest(std::move(request)), DataStore::kInvalidId);
}

TEST_F(DataStoreTest, BadBlacklistRegexIsRejected) {
  EXPECT_FALSE(datastore_->AddBlacklistRegex("["));
}

TEST_F(DataStoreTest, BadWhitelistRegexIsRejected) {
  EXPECT_FALSE(datastore_->AddWhitelistRegex("["));
}

TEST_F(DataStoreTest, AddRequestWithBlacklistedPathIsRejected) {
  std::unique_ptr<Request> request(new Request("http://example.org/oh/no"));
  EXPECT_TRUE(datastore_->AddBlacklistRegex("\\/oh\\/no"));
  EXPECT_EQ(datastore_->AddRequest(std::move(request)), DataStore::kInvalidId);
}

TEST_F(DataStoreTest, AddRequestWhitelistedOk) {
  std::unique_ptr<Request> request(new Request("http://example.org/oh"));
  EXPECT_TRUE(datastore_->AddWhitelistRegex("oh"));
  EXPECT_NE(datastore_->AddRequest(std::move(request)), DataStore::kInvalidId);
}

TEST_F(DataStoreTest, AddRequestNotWhitelistedIsRejected) {
  std::unique_ptr<Request> request(new Request("http://example.org/oh/no"));
  EXPECT_TRUE(datastore_->AddWhitelistRegex("something_else"));
  EXPECT_EQ(datastore_->AddRequest(std::move(request)), DataStore::kInvalidId);
}

TEST_F(DataStoreTest, AddRequestWhitelistedAndBlacklistMatchRejected) {
  std::unique_ptr<Request> request(
      new Request("http://example.org/oh/yes/reject/me/please"));
  EXPECT_TRUE(datastore_->AddWhitelistRegex("yes"));
  EXPECT_TRUE(datastore_->AddBlacklistRegex("re[a-z]ect"));
  EXPECT_EQ(datastore_->AddRequest(std::move(request)), DataStore::kInvalidId);
}

TEST_F(DataStoreTest, AddRequestWhichIsWhiteAndBlackListed) {
  std::unique_ptr<Request> request(new Request("http://example.org/oh/yes"));
  EXPECT_TRUE(datastore_->AddWhitelistRegex("\\/oh\\/yes"));
  EXPECT_TRUE(datastore_->AddBlacklistRegex("\\/oh\\/yes"));
  EXPECT_EQ(datastore_->AddRequest(std::move(request)), DataStore::kInvalidId);
}

TEST_F(DataStoreTest, AddResponseFingerprintToRequest) {
  std::unique_ptr<HtmlFingerprint> fingerprint(new HtmlFingerprint());

  EXPECT_CALL(*mock_request_, set_response_html_fingerprint(NotNull()));
  int64 id = datastore_->AddRequest(std::move(mock_request_));

  EXPECT_TRUE(
      datastore_->AddResponseFingerprintToRequest(id, std::move(fingerprint)));
}

TEST_F(DataStoreTest, AddResponseFingerprintToRequestFail) {
  std::unique_ptr<HtmlFingerprint> fingerprint(new HtmlFingerprint());
  datastore_->AddRequest(std::move(mock_request_));

  EXPECT_FALSE(datastore_->AddResponseFingerprintToRequest(
      4242, std::move(fingerprint)));
}

TEST_F(DataStoreTest, AddIssueOk) {
  std::string my_domain("foo.com");
  datastore_->AddHost(my_domain);
  const IssueDetails::IssueType issue_type = IssueDetails::MIXED_CONTENT;
  std::unique_ptr<Request> request_orig(new Request("http://" + my_domain));
  std::unique_ptr<Request> request_vuln(
      new Request("http://" + my_domain + "/vuln"));
  int64 id = datastore_->AddRequest(std::move(request_orig));
  EXPECT_TRUE(
      datastore_->AddIssue(id, issue_type, Severity::HIGH, request_vuln.get()));
  // Adding it a second time is prevented.
  EXPECT_FALSE(
      datastore_->AddIssue(id, issue_type, Severity::HIGH, request_vuln.get()));
  Request* stored_req = datastore_->GetRequestById(id);
  ASSERT_TRUE(stored_req != nullptr);
  auto& issue_map = stored_req->issues();
  ASSERT_EQ(issue_map.count(issue_type), 1);
  auto& issue_set = issue_map.find(issue_type)->second;
  ASSERT_EQ(issue_set.size(), 1);
  const IssueDetails* details = *issue_set.begin();
  EXPECT_EQ(details->severity(), Severity::HIGH);
}

TEST_F(DataStoreTest, AddMultipleIssuesOk) {
  std::string my_domain("foo.com");
  datastore_->AddHost(my_domain);
  std::unique_ptr<Request> request(new Request("http://" + my_domain));
  std::unique_ptr<Request> issue_request(
      new Request("http://" + my_domain + "/issue"));
  int64 id = datastore_->AddRequest(std::move(request));
  issue_request->set_parent_id(id);
  EXPECT_TRUE(datastore_->AddIssue(id, IssueDetails::MIXED_CONTENT,
                                   Severity::HIGH, issue_request.get()));
  EXPECT_TRUE(datastore_->AddIssue(id, IssueDetails::XSS_STORED, Severity::HIGH,
                                   issue_request.get()));

  Request* stored_req = datastore_->GetRequestById(id);
  ASSERT_TRUE(stored_req != nullptr);
  ASSERT_EQ(stored_req->issues().size(), 2);
}

// Same as above except that the enforced limit will prevent the second
// issue from being added.
TEST_F(DataStoreTest, AddIssueLimitPerUrl) {
  std::string my_domain("foo.com");
  absl::SetFlag(&FLAGS_max_issues_per_url, 1);
  datastore_->AddHost(my_domain);
  std::unique_ptr<Request> request(new Request("http://" + my_domain));
  std::unique_ptr<Request> issue_request(
      new Request("http://" + my_domain + "/issue"));
  int64 id = datastore_->AddRequest(std::move(request));
  issue_request->set_parent_id(id);
  EXPECT_TRUE(datastore_->AddIssue(id, IssueDetails::MIXED_CONTENT,
                                   Severity::HIGH, issue_request.get()));
  EXPECT_FALSE(datastore_->AddIssue(id, IssueDetails::XSS_STORED,
                                    Severity::HIGH, issue_request.get()));
  Request* stored_req = datastore_->GetRequestById(id);
  ASSERT_TRUE(stored_req != nullptr);
  ASSERT_EQ(stored_req->issues().size(),
            absl::GetFlag(FLAGS_max_issues_per_url));
}

TEST_F(DataStoreTest, AddIssueLimitPerCheck) {
  std::string my_domain("foo.com");
  const IssueDetails::IssueType issue_type = IssueDetails::MIXED_CONTENT;
  absl::SetFlag(&FLAGS_max_issues_per_check_per_url, 2);
  datastore_->AddHost(my_domain);
  std::unique_ptr<Request> request(new Request("http://" + my_domain));
  std::unique_ptr<Request> issue_request(
      new Request("http://" + my_domain + "/issue"));
  int64 id = datastore_->AddRequest(std::move(request));
  issue_request->set_parent_id(id);
  EXPECT_TRUE(datastore_->AddIssue(id, issue_type, Severity::HIGH,
                                   issue_request.get()));
  EXPECT_TRUE(datastore_->AddIssue(id, issue_type, Severity::CRITICAL,
                                   issue_request.get()));
  EXPECT_FALSE(
      datastore_->AddIssue(id, issue_type, Severity::LOW, issue_request.get()));
  Request* stored_req = datastore_->GetRequestById(id);
  ASSERT_TRUE(stored_req != nullptr);
  ASSERT_EQ(stored_req->issues().size(), 1);
  auto& issue_set = stored_req->issues().find(issue_type)->second;
  EXPECT_EQ(issue_set.size(),
            absl::GetFlag(FLAGS_max_issues_per_check_per_url));
}

TEST_F(DataStoreTest, AddIssueHitsGlobalCheckLimit) {
  std::string my_domain("foo.com");
  const IssueDetails::IssueType issue_type = IssueDetails::MIXED_CONTENT;
  absl::SetFlag(&FLAGS_max_issues_per_check, 1);
  datastore_->AddHost(my_domain);
  std::unique_ptr<Request> request(new Request("http://" + my_domain));
  std::unique_ptr<Request> issue_request(
      new Request("http://" + my_domain + "/issue"));
  int64 id = datastore_->AddRequest(std::move(request));
  issue_request->set_parent_id(id);
  EXPECT_TRUE(datastore_->AddIssue(id, issue_type, Severity::HIGH,
                                   issue_request.get()));
  EXPECT_FALSE(datastore_->AddIssue(id, issue_type, Severity::CRITICAL,
                                    issue_request.get()));
  EXPECT_FALSE(
      datastore_->AddIssue(id, issue_type, Severity::LOW, issue_request.get()));
  Request* stored_req = datastore_->GetRequestById(id);
  ASSERT_TRUE(stored_req != nullptr);
  ASSERT_EQ(stored_req->issues().size(), 1);
  auto& issue_set = stored_req->issues().find(issue_type)->second;
  EXPECT_EQ(issue_set.size(), absl::GetFlag(FLAGS_max_issues_per_check));
}

TEST_F(DataStoreTest, RequestReportOk) {
  // Ensure there is something to report.
  std::unique_ptr<Request> request_one(new Request("http://example.org/one/"));
  std::unique_ptr<Request> request_two(
      new Request("http://example.org/one/two/three"));

  EXPECT_GT(datastore_->AddRequest(std::move(request_one)), 0);
  EXPECT_GT(datastore_->AddRequest(std::move(request_two)), 0);

  testing::MockReporter* mock_reporter = new testing::MockReporter();
  EXPECT_CALL(*mock_reporter,
              ReportPivot(Property(&Pivot::name, StrEq("example.org")), 1));
  EXPECT_CALL(*mock_reporter,
              ReportPivot(Property(&Pivot::name, StrEq("one")), 2));
  EXPECT_CALL(*mock_reporter,
              ReportPivot(Property(&Pivot::name, StrEq("")), 3));
  EXPECT_CALL(*mock_reporter,
              ReportPivot(Property(&Pivot::name, StrEq("two")), 3));
  EXPECT_CALL(*mock_reporter,
              ReportPivot(Property(&Pivot::name, StrEq("three")), 4));
  std::vector<std::unique_ptr<ReporterInterface>> reporters;
  reporters.push_back(std::unique_ptr<ReporterInterface>(mock_reporter));
  datastore_->Report(reporters);
}

TEST_F(DataStoreTest, RequestRejectedByPivot) {
  absl::SetFlag(&FLAGS_pivot_child_limit, 1);
  std::unique_ptr<Request> request_one(new Request("http://example.org/ok"));
  std::unique_ptr<Request> request_two(new Request("http://example.org/nok"));
  EXPECT_GT(datastore_->AddRequest(std::move(request_one)), 0);
  EXPECT_EQ(datastore_->AddRequest(std::move(request_two)),
            DataStore::kInvalidId);
}

TEST_F(DataStoreTest, AddRequestMetadataOk) {
  std::unique_ptr<Request> request(new Request("http://example.org/see/you"));
  int64 request_id = datastore_->AddRequest(std::move(request));

  ASSERT_THAT(datastore_->GetRequestById(request_id), NotNull());
  int64 response_time_to_set = 10;
  EXPECT_TRUE(datastore_->AddRequestMetadata(
      request_id, MetaData_Type::MetaData_Type_AVERAGE_APPLICATION_TIME_USEC,
      response_time_to_set));

  int64 response_time_fetched = 0;
  EXPECT_TRUE(datastore_->GetRequestMetadata(
      request_id, MetaData_Type::MetaData_Type_AVERAGE_APPLICATION_TIME_USEC,
      &response_time_fetched));
  EXPECT_EQ(response_time_to_set, response_time_fetched);
}

TEST_F(DataStoreTest, AddRequestMetadataAddsOnlyOnce) {
  std::unique_ptr<Request> request(new Request("http://example.org/see/you"));

  int64 request_id = datastore_->AddRequest(std::move(request));
  ASSERT_THAT(datastore_->GetRequestById(request_id), NotNull());
  EXPECT_TRUE(datastore_->AddRequestMetadata(
      request_id, MetaData_Type::MetaData_Type_AVERAGE_APPLICATION_TIME_USEC,
      42));
  EXPECT_FALSE(datastore_->AddRequestMetadata(
      request_id, MetaData_Type::MetaData_Type_AVERAGE_APPLICATION_TIME_USEC,
      43));
}

TEST_F(DataStoreTest, GetRequestMetadataNoContent) {
  int64 get_value = 0;
  EXPECT_FALSE(datastore_->GetRequestMetadata(
      42, MetaData_Type::MetaData_Type_AVERAGE_APPLICATION_TIME_USEC,
      &get_value));
}

TEST_F(DataStoreTest, AddRequestToAuditQueueOk) {
  Request req("http://example.org");
  EXPECT_EQ(0, datastore_->audit_queue_size());
  datastore_->AddRequestToAuditQueue(&req);
  EXPECT_EQ(1, datastore_->audit_queue_size());
  EXPECT_EQ(&req, datastore_->GetRequestFromAuditQueue());
  EXPECT_EQ(0, datastore_->audit_queue_size());
}

TEST_F(DataStoreTest, GetRequestFromAuditQueueIsFifo) {
  Request first_request("http://example.org/first");
  Request second_request("http://example.org/second");
  EXPECT_EQ(0, datastore_->audit_queue_size());
  datastore_->AddRequestToAuditQueue(&first_request);
  datastore_->AddRequestToAuditQueue(&second_request);
  EXPECT_EQ(2, datastore_->audit_queue_size());
  EXPECT_EQ(&first_request, datastore_->GetRequestFromAuditQueue());
  EXPECT_EQ(&second_request, datastore_->GetRequestFromAuditQueue());
  EXPECT_EQ(0, datastore_->audit_queue_size());
}

TEST_F(DataStoreTest, GetRequestFromAuditQueueEmpty) {
  EXPECT_EQ(nullptr, datastore_->GetRequestFromAuditQueue());
}

}  // namespace plusfish
