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

#include "parsers/gumbo_form_filter.h"

#include <memory>

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "absl/container/node_hash_set.h"
#include <gumbo.h>
#include "parsers/gumbo_filter.h"
#include "parsers/gumbo_parser.h"
#include "proto/http_request.pb.h"
#include "proto/issue_details.pb.h"
#include "response.h"
#include "testing/request_mock.h"

using testing::HasSubstr;
using testing::Return;
using testing::ReturnRef;

namespace plusfish {

// A helper method to create an HTML form string for the tests.
std::string CreateFormString(
    std::string action_url,
    const std::map<std::string, std::string>& text_fields,
    const std::string* method, bool append_end_tag) {
  std::string form_string("<form action='" + action_url + "'");
  if (method == nullptr) {
    form_string.append(">");
  } else {
    form_string.append(" method='" + *method + "'>");
  }

  for (const auto& field_iter : text_fields) {
    form_string.append("<input type='text' name='" + field_iter.first + "' ");
    form_string.append("value='" + field_iter.second + "'>");
  }
  if (append_end_tag) {
    form_string.append("</form>");
  }
  return form_string;
}

// A helper method to test whether the given form_request parsed all content
// from the given test form correctly.
void ValidateFormRequest(const Request* request, const std::string& form_url,
                         const std::map<std::string, std::string>& fields) {
  int body_param_count = request->proto().body_param_size();
  EXPECT_EQ(fields.size(), body_param_count);

  for (int p_index = 0; p_index < body_param_count; ++p_index) {
    HttpRequest::RequestField param = request->proto().body_param(p_index);
    // Test if a parameter with matching name is present.
    const auto& field_iter = fields.find(param.name());
    EXPECT_NE(field_iter, fields.end());
    // Test that the matching parameter has a matching value.
    EXPECT_EQ(field_iter->second, param.value());
  }
}

class HtmlFormFilterTest : public ::testing::Test {
 protected:
  HtmlFormFilterTest() : form_request_url_("https://example.org") {}

  void SetUp() override {
    form_request_.reset(new testing::MockRequest(form_request_url_));
    ON_CALL(*form_request_, url()).WillByDefault(ReturnRef(form_request_url_));
    form_filter_.reset(
        new GumboFormFilter(form_request_.get(), &requests_, &issues_));
    filters_.emplace_back(form_filter_.get());
    gumbo_.reset(new GumboParser());
  }

  void ParseAndFilter(const std::string& content_to_parse) {
    response_.reset(new Response());
    response_->set_body(content_to_parse);
    ON_CALL(*form_request_, response()).WillByDefault(Return(response_.get()));
    gumbo_->Parse(content_to_parse);
    gumbo_->FilterDocument(filters_);
  }

  std::string form_request_url_;
  std::unique_ptr<GumboFormFilter> form_filter_;
  std::unique_ptr<GumboParser> gumbo_;
  std::unique_ptr<testing::MockRequest> form_request_;
  std::unique_ptr<Response> response_;
  absl::node_hash_set<std::unique_ptr<IssueDetails>> issues_;
  std::vector<GumboFilter*> filters_;
  std::vector<std::unique_ptr<Request>> requests_;
  std::vector<std::string> anchors_;
};

TEST_F(HtmlFormFilterTest, ParseContentWithoutForm) {
  gumbo_->Parse("<a href='hello'>hello</a>");
  gumbo_->FilterDocument(filters_);
  EXPECT_EQ(0, requests_.size());
}

TEST_F(HtmlFormFilterTest, ParseContentWithEmptyForm) {
  gumbo_->Parse("<form/>");
  gumbo_->FilterDocument(filters_);
  EXPECT_EQ(0, requests_.size());
}

TEST_F(HtmlFormFilterTest, ParseInputFieldWithoutForm) {
  gumbo_->Parse("<input type='text'/>");
  gumbo_->FilterDocument(filters_);
  EXPECT_EQ(0, requests_.size());
}

TEST_F(HtmlFormFilterTest, ParseFormWithRelativeUrl) {
  std::map<std::string, std::string> form_fields = {
      {"field1", "value1"}, {"field2", "value2"}, {"field3", ""}};
  std::string action_url = "/foo";

  ParseAndFilter(CreateFormString(action_url, form_fields, nullptr, true));
  std::string parsed_action_url = form_request_->url() + "foo";
  ValidateFormRequest(requests_[0].get(), parsed_action_url, form_fields);
}

TEST_F(HtmlFormFilterTest, ParseFormWithAbsoluteUrl) {
  std::map<std::string, std::string> form_fields = {
      {"field1", "value1"}, {"field2", "value2"}, {"field3", ""}};

  std::string action_url = "http://different.example.org/foo";
  ParseAndFilter(CreateFormString(action_url, form_fields, nullptr, true));
  ValidateFormRequest(requests_[0].get(), action_url, form_fields);
}

TEST_F(HtmlFormFilterTest, ParseFormWithExplicitPostMethod) {
  std::string method("POST");
  std::map<std::string, std::string> form_fields = {
      {"field1", "value1"}, {"field2", "value2"}, {"field3", ""}};

  std::string action_url = "http://different.example.org/foo";
  ParseAndFilter(CreateFormString(action_url, form_fields, &method, true));
  ValidateFormRequest(requests_[0].get(), action_url, form_fields);
  CHECK_EQ(requests_[0]->proto().method(), HttpRequest::POST);
}

TEST_F(HtmlFormFilterTest, ParseFormWithHtmlEscapedActionUrl) {
  std::string method("POST");
  std::map<std::string, std::string> form_fields = {
      {"field1", "value1"},
  };

  std::string action_url = "http://different.example.org/foo&amp;";
  ParseAndFilter(CreateFormString(action_url, form_fields, &method, true));
  EXPECT_STREQ(requests_[0]->url().c_str(),
               "http://different.example.org:80/foo&");
}

TEST_F(HtmlFormFilterTest, ParseFormUnknownMethodDefaultsToPost) {
  std::string method("PUT");
  std::map<std::string, std::string> form_fields = {
      {"field1", "value1"}, {"field2", "value2"}, {"field3", ""}};

  std::string action_url = "http://different.example.org/foo";
  ParseAndFilter(CreateFormString(action_url, form_fields, &method, true));
  ValidateFormRequest(requests_[0].get(), action_url, form_fields);
}

TEST_F(HtmlFormFilterTest, ParseFormWithExplicitGetMethod) {
  std::string method("GET");
  std::map<std::string, std::string> form_fields = {
      {"field1", "value1"}, {"field2", "value2"}, {"field3", ""}};

  std::string action_url = "http://different.example.org/foo";
  ParseAndFilter(CreateFormString(action_url, form_fields, &method, true));

  EXPECT_EQ(0, requests_[0]->proto().body_param_size());
  EXPECT_EQ(form_fields.size(), requests_[0]->proto().param_size());
}

// This test is slightly redundant as it mostly guards the gumbo parser
// behavior regarding the handling of missing end-tags. Still really want this
// test to fail when that behavior changes.
TEST_F(HtmlFormFilterTest, ParseFormWithoutEndTag) {
  std::map<std::string, std::string> form_fields = {
      {"field1", "value1"}, {"field2", "value2"}, {"field3", ""}};

  std::string action_url = "http://www.example.org/foo";
  ParseAndFilter(CreateFormString(action_url, form_fields, nullptr, false));
  ValidateFormRequest(requests_[0].get(), action_url, form_fields);
}

TEST_F(HtmlFormFilterTest, ParseFormWithEmptyActionUrl) {
  std::map<std::string, std::string> form_fields = {{"field1", "value1"},
                                                    {"field2", ""}};

  ParseAndFilter(CreateFormString("", form_fields, nullptr, true));
  std::string expected_url = form_request_->url();
  ValidateFormRequest(requests_[0].get(), expected_url, form_fields);
}

TEST_F(HtmlFormFilterTest, ParseFormWithEmptyNameInputField) {
  std::map<std::string, std::string> form_fields = {{"", "nameless"},
                                                    {"", "ismore"}};

  ParseAndFilter(CreateFormString("", form_fields, nullptr, true));
  // The gumbo parser accepts form fields that have no names. The pagespeed
  // parser did not do this and would fail on this test.
  EXPECT_EQ(1, requests_[0]->proto().body_param_size());
}

TEST_F(HtmlFormFilterTest, ParseFormWithDuplicateInputField) {
  std::map<std::string, std::string> form_fields = {{"twin", "nameless"},
                                                    {"twin", "ismore"}};

  ParseAndFilter(CreateFormString("", form_fields, nullptr, true));
  std::string expected_url = form_request_->url();
  ValidateFormRequest(requests_[0].get(), expected_url, form_fields);
}

TEST_F(HtmlFormFilterTest, ParseFormWithUnknownInputFieldType) {
  std::map<std::string, std::string> form_fields = {{"field_one", "value_one"},
                                                    {"field_two", "value_two"}};

  std::string form_string = CreateFormString("", form_fields, nullptr, false);
  form_string.append("<input type='this_will_be_ignored'>");

  ParseAndFilter(form_string);
  std::string expected_url = form_request_->url();
  ValidateFormRequest(requests_[0].get(), expected_url, form_fields);
}

TEST_F(HtmlFormFilterTest, ParseFormWithTextAreaInputFieldType) {
  std::map<std::string, std::string> form_input_fields = {
      {"field_one", "value_one"}, {"field_two", "value_two"}};

  std::string form_string =
      CreateFormString("", form_input_fields, nullptr, false);
  form_string.append("<textarea name='field_three'></textarea>");

  ParseAndFilter(form_string);
  std::string expected_url = form_request_->url();
  EXPECT_EQ(3, requests_[0]->proto().body_param_size());
  EXPECT_STREQ("field_three",
               requests_[0]->proto().body_param(2).name().c_str());
}

TEST_F(HtmlFormFilterTest, ParseFormWithMalformedTargetUrl) {
  std::map<std::string, std::string> form_fields = {{"name", "value"}};

  ParseAndFilter(CreateFormString("wut://aa", form_fields, nullptr, true));
  std::string expected_url = form_request_->url();
  ValidateFormRequest(requests_[0].get(), expected_url, form_fields);
}

TEST_F(HtmlFormFilterTest, ParseFormDetectsPasswordSubmissionWithoutSsl) {
  std::map<std::string, std::string> form_input_fields = {
      {"field_one", "value_one"}, {"field_two", "value_two"}};

  std::string action_url = "http://www.example.org:80/insecure";
  std::string form_string =
      CreateFormString(action_url, form_input_fields, nullptr, false);
  form_string.append(
      "<input type='hidden' name='x' value='aGVsbG93b3JsZA=='></input>");
  form_string.append("<input type='password' name='pass'></input>");
  ParseAndFilter(form_string);
  ASSERT_EQ(1, issues_.size());
  EXPECT_EQ(issues_.begin()->get()->type(),
            IssueDetails::PLAINTEXT_LOGIN_FORM_TARGET);
  EXPECT_THAT(issues_.begin()->get()->extra_info(), HasSubstr(action_url));
}

TEST_F(HtmlFormFilterTest, ParseFormDetectsPasswordFormServedWithoutSsl) {
  std::map<std::string, std::string> form_input_fields = {
      {"field_one", "value_one"}, {"field_two", "value_two"}};

  form_request_->ParseUrl("http://example.org/plaintext_login_form", nullptr);
  std::string action_url = "https://www.example.org/insecure";
  std::string form_string =
      CreateFormString(action_url, form_input_fields, nullptr, false);
  form_string.append(
      "<input type='hidden' name='x' value='aGVsbG93b3JsZA=='></input>");
  form_string.append("<input type='password' name='pass'></input>");
  ParseAndFilter(form_string);
  ASSERT_EQ(1, issues_.size());
  EXPECT_EQ(issues_.begin()->get()->type(), IssueDetails::PLAINTEXT_LOGIN_FORM);
}

TEST_F(HtmlFormFilterTest, ParseFormIgnoresSSLProtectedPasswordForm) {
  std::map<std::string, std::string> form_input_fields = {
      {"field_one", "value_one"}, {"field_two", "value_two"}};

  std::string action_url = "https://www.example.org/secure";
  std::string form_string =
      CreateFormString(action_url, form_input_fields, nullptr, false);
  form_string.append("<input type='password' name='pass'></input>");
  form_string.append(
      "<input type='hidden' name='x' value='aGVsbG93b3JsZA=='></input>");
  ParseAndFilter(form_string);
  ASSERT_EQ(0, issues_.size());
}

TEST_F(HtmlFormFilterTest, ParseTwoFormsAndDetectOneIssue) {
  std::map<std::string, std::string> form_input_fields = {
      {"field_one", "value_one"}, {"field_two", "value_two"}};

  std::string action_url = "http://www.example.org/insecure";
  std::string form_without_password =
      CreateFormString(action_url, form_input_fields, nullptr, false);
  form_without_password.append(
      "<input type='hidden' name='x' value='aGVsbG93b3JsZA=='></input>");

  std::string form_with_password = form_without_password;
  form_with_password.append("<input type='password' name='pass'></input>");

  ParseAndFilter(form_with_password);
  ParseAndFilter(form_without_password);
  ASSERT_EQ(1, issues_.size());
  EXPECT_EQ(issues_.begin()->get()->type(),
            IssueDetails::PLAINTEXT_LOGIN_FORM_TARGET);
}

TEST_F(HtmlFormFilterTest, ParseFormRaisesMissingXsrfIssue) {
  std::map<std::string, std::string> form_input_fields = {
      {"field_one", "value_one"}, {"field_two", "value_two"}};

  std::string action_url = "https://www.example.org/secure";
  std::string form_string =
      CreateFormString(action_url, form_input_fields, nullptr, false);
  form_string.append("<input type='password' name='pass'></input>");
  ParseAndFilter(form_string);
  ASSERT_EQ(1, issues_.size());
  EXPECT_EQ(issues_.begin()->get()->type(), IssueDetails::XSRF_PASSIVE);
}

TEST_F(HtmlFormFilterTest, ParseFormIsHappyWithXsrfToken) {
  std::map<std::string, std::string> form_input_fields = {
      {"field_one", "value_one"}, {"field_two", "value_two"}};

  std::string action_url = "https://www.example.org/secure";
  std::string form_string =
      CreateFormString(action_url, form_input_fields, nullptr, false);
  form_string.append(
      "<input type='hidden' name='xsrf' value='aGVsbG93b3JsZA=='></input>");
  ParseAndFilter(form_string);
  ASSERT_EQ(0, issues_.size());
}

TEST_F(HtmlFormFilterTest, ParseFormRaisesXsrfWhenNoSuitableTokenExists) {
  std::map<std::string, std::string> form_input_fields = {
      {"field_one", "value_one"}, {"field_two", "value_two"}};

  std::string action_url = "https://www.example.org/secure";
  std::string form_string =
      CreateFormString(action_url, form_input_fields, nullptr, false);
  form_string.append("<input type='password' name='pass'></input>");
  form_string.append("<input type='hidden' name='xsrf' value='nope'></input>");
  ParseAndFilter(form_string);
  ASSERT_EQ(1, issues_.size());
  EXPECT_EQ(issues_.begin()->get()->type(), IssueDetails::XSRF_PASSIVE);
}

TEST_F(HtmlFormFilterTest, ParseFormWithNestedElements) {
  std::map<std::string, std::string> form_input_fields = {
      {"field_one", "value_one"}, {"field_two", "value_two"}};

  std::string action_url = "https://www.example.org/secure";
  std::string form_string =
      CreateFormString(action_url, form_input_fields, nullptr, false);
  form_string.append(
      "<div><div><input type='text' name='nested_field'></input></div></div>");
  ParseAndFilter(form_string);
  EXPECT_EQ(3, requests_[0]->proto().body_param_size());
  EXPECT_STREQ("nested_field",
               requests_[0]->proto().body_param(2).name().c_str());
}

}  // namespace plusfish
