#include "hidden_objects_finder.h"

#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/node_hash_map.h"
#include "absl/strings/str_split.h"
#include "parsers/gumbo_filter.h"
#include "parsers/gumbo_fingerprint_filter.h"
#include "parsers/gumbo_parser.h"
#include "proto/http_response.pb.h"
#include "proto/issue_details.pb.h"
#include "request.h"
#include "testing/request_mock.h"
#include "util/url.h"

using testing::Return;
using testing::ReturnNull;
using testing::UnorderedElementsAreArray;

namespace plusfish {

class HiddenObjectsFinderTest : public ::testing::Test {
 public:
  HiddenObjectsFinderTest()
      : fake_schedule_return_value_(true),
        fake_is_html_fingerprint_return_value_(false),
        fake_is_html_fingerprint_called_(false),
        fake_add_request_return_value_(42),
        fake_add_request_called_(false),
        fake_add_issue_return_value_(false),
        fake_add_issue_called_(false) {}

  void SetUp() override {
    objects_finder_.reset(new HiddenObjectsFinder(
        [this](const Request* req) {
          if (fake_schedule_return_value_) {
            this->fake_schedule_req_urls_.insert(req->url());
          }
          return fake_schedule_return_value_;
        },

        [this](const HtmlFingerprint* fp) {
          fake_is_html_fingerprint_called_ = true;
          return fake_is_html_fingerprint_return_value_;
        },
        [this](std::unique_ptr<Request> req) {
          fake_add_request_requests_.emplace_back(std::move(req));
          fake_add_request_called_ = true;
          return fake_add_request_return_value_;
        },
        [this](const int64, const IssueDetails::IssueType, const Severity) {
          fake_add_issue_called_ = true;
          return fake_add_issue_return_value_;
        }));
  }

  // Schedule request callback parameters.
  absl::flat_hash_set<std::string> fake_schedule_req_urls_;
  bool fake_schedule_return_value_;
  // Is 404 HtmlFingerprint callback parameters.
  bool fake_is_html_fingerprint_return_value_;
  bool fake_is_html_fingerprint_called_;
  // Add Request callback parameters.
  int fake_add_request_return_value_;
  int fake_add_request_called_;
  std::vector<std::unique_ptr<Request>> fake_add_request_requests_;
  std::unique_ptr<HiddenObjectsFinder> objects_finder_;
  // Add issue callback parameters.
  bool fake_add_issue_return_value_;
  bool fake_add_issue_called_;
};

TEST_F(HiddenObjectsFinderTest, AddWordListLineOk) {
  EXPECT_TRUE(objects_finder_->AddWordlistLine("word 1"));
  EXPECT_TRUE(objects_finder_->AddWordlistLine("word 0"));
  EXPECT_TRUE(objects_finder_->AddWordlistLine("word\t0"));
}

TEST_F(HiddenObjectsFinderTest, AddWordListLineNotOk) {
  // Missing 1 or 2.
  EXPECT_FALSE(objects_finder_->AddWordlistLine("word"));
  EXPECT_FALSE(objects_finder_->AddWordlistLine("word not_a_digit"));
  // Too many digits.
  EXPECT_FALSE(objects_finder_->AddWordlistLine("word 42"));
}

TEST_F(HiddenObjectsFinderTest, GenerateTestUrlsOK) {
  EXPECT_TRUE(objects_finder_->AddWordlistLine("word 1"));
  EXPECT_TRUE(objects_finder_->AddExtension(".php"));
  EXPECT_TRUE(objects_finder_->AddExtension(".pl"));
  objects_finder_->AddUrl("http://example.org/foo");
  EXPECT_EQ(fake_schedule_req_urls_.size(),
            objects_finder_->ScheduleRequests(3));
  EXPECT_EQ(fake_schedule_req_urls_.size(), 3);
  EXPECT_THAT(fake_schedule_req_urls_,
              UnorderedElementsAreArray({"http://example.org:80/word",
                                         "http://example.org:80/word.php",
                                         "http://example.org:80/word.pl"}));
}

TEST_F(HiddenObjectsFinderTest, GenerateTestUrlsLimitedAmountOK) {
  EXPECT_TRUE(objects_finder_->AddWordlistLine("word 1"));
  EXPECT_TRUE(objects_finder_->AddWordlistLine("ward 1"));
  EXPECT_TRUE(objects_finder_->AddExtension(".php"));
  EXPECT_TRUE(objects_finder_->AddExtension(".pl"));
  objects_finder_->AddUrl("http://example.org/foo");
  objects_finder_->ScheduleRequests(3);
  EXPECT_EQ(fake_schedule_req_urls_.size(), 3);
  EXPECT_THAT(fake_schedule_req_urls_,
              UnorderedElementsAreArray({"http://example.org:80/word.php",
                                         "http://example.org:80/word.pl",
                                         "http://example.org:80/word"}));
}

TEST_F(HiddenObjectsFinderTest, AddUrlDoesNotScheduleDuplicates) {
  EXPECT_TRUE(objects_finder_->AddWordlistLine("tar 1"));
  EXPECT_TRUE(objects_finder_->AddWordlistLine("tar.gz 1"));
  EXPECT_TRUE(objects_finder_->AddExtension(".gz"));
  objects_finder_->AddUrl("http://example.org/foo");
  EXPECT_EQ(fake_schedule_req_urls_.size(),
            objects_finder_->ScheduleRequests(5));
  EXPECT_EQ(fake_schedule_req_urls_.size(), 3);
  // There will only be one tar.gz scheduled.
  EXPECT_THAT(fake_schedule_req_urls_,
              UnorderedElementsAreArray({"http://example.org:80/tar",
                                         "http://example.org:80/tar.gz",
                                         "http://example.org:80/tar.gz.gz"}));
}

TEST_F(HiddenObjectsFinderTest, AddUrlDoesntSchedulExtensionRequestsOk) {
  EXPECT_TRUE(objects_finder_->AddWordlistLine("word 1"));
  EXPECT_TRUE(objects_finder_->AddWordlistLine("drow 0"));
  EXPECT_TRUE(objects_finder_->AddExtension(".php"));
  EXPECT_TRUE(objects_finder_->AddExtension(".pl"));
  objects_finder_->AddUrl("http://example.org/foo");
  EXPECT_EQ(fake_schedule_req_urls_.size(),
            objects_finder_->ScheduleRequests(4));
  EXPECT_EQ(fake_schedule_req_urls_.size(), 4);
  EXPECT_THAT(
      fake_schedule_req_urls_,
      UnorderedElementsAreArray(
          {"http://example.org:80/word", "http://example.org:80/drow",
           "http://example.org:80/word.php", "http://example.org:80/word.pl"}));
}

TEST_F(HiddenObjectsFinderTest, AddUrlFailsOnSchedule) {
  EXPECT_TRUE(objects_finder_->AddWordlistLine("word 1"));
  EXPECT_TRUE(objects_finder_->AddExtension(".php"));
  EXPECT_TRUE(objects_finder_->AddExtension(".pl"));
  fake_schedule_return_value_ = false;
  objects_finder_->AddUrl("http://example.org/foo");
  EXPECT_EQ(fake_schedule_req_urls_.size(),
            objects_finder_->ScheduleRequests(4));

  EXPECT_EQ(fake_schedule_req_urls_.size(), 0);
}

TEST_F(HiddenObjectsFinderTest, RequestCallbackNoResponse) {
  testing::MockRequest request("http://test:80/foo.html");
  EXPECT_CALL(request, response()).WillOnce(ReturnNull());
  EXPECT_EQ(0, objects_finder_->RequestCallback(&request));
}

TEST_F(HiddenObjectsFinderTest, RequestCallbackOk) {
  testing::MockRequest request("http://test:80/foo.html");

  Response response;
  response.Parse(
      "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
      "<html><p>a la la</p></html>");

  EXPECT_CALL(request, response()).Times(4).WillRepeatedly(Return(&response));
  EXPECT_EQ(1, objects_finder_->RequestCallback(&request));
  EXPECT_TRUE(fake_add_request_called_);
  EXPECT_EQ(fake_add_request_requests_.size(), 1);
  EXPECT_TRUE(fake_add_issue_called_);
}

TEST_F(HiddenObjectsFinderTest, RequestNotAddedByDatastoreOk) {
  // If a found URL is out of scope, it will not be added to the datastore and
  // the AddRequest callback will return a negative device ID.
  testing::MockRequest request("http://test:80/foo.html");

  Response response;
  response.Parse(
      "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
      "<html><p>a la la</p></html>");

  fake_add_request_return_value_ = -1;
  EXPECT_CALL(request, response()).Times(4).WillRepeatedly(Return(&response));
  EXPECT_EQ(1, objects_finder_->RequestCallback(&request));
  EXPECT_TRUE(fake_add_request_called_);
  EXPECT_FALSE(fake_add_issue_called_);
}

TEST_F(HiddenObjectsFinderTest, RequestCallbackIs404Fingerprint) {
  testing::MockRequest request("http://test:80/foo.html");

  Response response;
  response.Parse(
      "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
      "<html><p>a la la</p></html>");

  fake_is_html_fingerprint_return_value_ = true;
  EXPECT_CALL(request, response()).Times(4).WillRepeatedly(Return(&response));
  EXPECT_EQ(0, objects_finder_->RequestCallback(&request));
  EXPECT_FALSE(fake_add_request_called_);
  EXPECT_FALSE(fake_add_issue_called_);
}

TEST_F(HiddenObjectsFinderTest, RequestCallbackGets404Request) {
  testing::MockRequest request("http://test:80/foo.html");

  Response response;
  response.Parse(
      "HTTP/1.0 404 OK\r\nContent-Type: text/html\r\n\r\n"
      "<html><p>a la la</p></html>");

  EXPECT_CALL(request, response()).Times(2).WillRepeatedly(Return(&response));
  EXPECT_EQ(1, objects_finder_->RequestCallback(&request));
  EXPECT_FALSE(fake_is_html_fingerprint_called_);
  EXPECT_FALSE(fake_add_request_called_);
  EXPECT_FALSE(fake_add_issue_called_);
}

}  // namespace plusfish
