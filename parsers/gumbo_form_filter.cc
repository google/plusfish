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

#include <string.h>

#include <algorithm>
#include <cstring>
#include <string>

#include <glog/logging.h>
#include "absl/container/node_hash_set.h"
#include <gumbo.h>
#include "audit/util/issue_util.h"
#include "parsers/gumbo_parser.h"
#include "parsers/util/escaping.h"
#include "parsers/util/html_name.h"
#include "parsers/util/scrape_util.h"
#include "proto/issue_details.pb.h"
#include "request.h"

// The HTTP POST method string value.
static const char* kGetMethod = "GET";
// The type of a password input field.
static const char* kPasswordFieldType = "password";
// The type of a hidden field.
static const char* kHiddenFieldType = "hidden";

namespace plusfish {

GumboFormFilter::GumboFormFilter()
    : form_response_body_offset_(0LL),
      form_method_is_post_(true),
      form_has_password_field_(false),
      form_has_xsrf_token_(false),
      base_request_(nullptr),
      requests_(nullptr),
      issues_(nullptr) {}

GumboFormFilter::GumboFormFilter(
    const Request* base_request,
    std::vector<std::unique_ptr<Request>>* requests,
    absl::node_hash_set<std::unique_ptr<IssueDetails>>* issues)
    : form_response_body_offset_(0LL),
      form_method_is_post_(true),
      form_has_password_field_(false),
      form_has_xsrf_token_(false),
      base_request_(base_request),
      requests_(requests),
      issues_(issues) {}

GumboFormFilter::~GumboFormFilter() {}

void GumboFormFilter::AnalyzeCurrentForm() {
  if (!current_request_) {
    return;
  }
  if (form_has_password_field_) {
    // Is the target lacking SSL protection?
    if (!current_request_->proto().ssl()) {
      util::UpdateIssueVectorWithSnippet(
          IssueDetails::PLAINTEXT_LOGIN_FORM_TARGET, Severity::HIGH,
          base_request_->id(), base_request_->raw_response(),
          form_response_body_offset_, "Form target: " + current_request_->url(),
          issues_);
    }

    // Is the password form served without SSL protection?
    if (!base_request_->proto().ssl()) {
      util::UpdateIssueVectorWithSnippet(
          IssueDetails::PLAINTEXT_LOGIN_FORM, Severity::HIGH,
          base_request_->id(), base_request_->response()->body(),
          form_response_body_offset_, "", issues_);
    }
  }

  // Report forms that lack a potential XSRF token.
  if (!form_has_xsrf_token_) {
    util::UpdateIssueVectorWithSnippet(IssueDetails::XSRF_PASSIVE,
                                       Severity::MODERATE, base_request_->id(),
                                       base_request_->response()->body(),
                                       form_response_body_offset_, "", issues_);
  }
}

void GumboFormFilter::ParseElement(const GumboElement& node) {
  // Return immediately if this isn't the start of a form.
  if (node.tag != GUMBO_TAG_FORM) {
    return;
  }

  // Try to find a target URL for the new form request.
  const GumboAttribute* action_attr =
      GumboParser::GetAttribute(&node.attributes, HTMLName::kAction);

  if (action_attr == nullptr) {
    DLOG(WARNING) << "Line " << node.start_pos.line
                  << ": Form has no action attribute. ";
    // TODO: Implement usage of the current URL as the default form
    // action target.
    return;
  }

  std::string unescaped_url = util::UnescapeHtml(action_attr->value);
  current_request_.reset(new Request(unescaped_url, base_request_));
  form_response_body_offset_ = node.start_pos.offset;
  DLOG(INFO) << "Created new form request for target: " << unescaped_url;

  // For new forms we don't know yet if it carries a password field.
  form_has_password_field_ = false;

  // Extract the form method.
  // The default is POST so we'll assume this is the case and will try to parse
  // the 'method' attribute from the form to see if GET should be used.
  form_method_is_post_ = true;
  const GumboAttribute* method_attr =
      GumboParser::GetAttribute(&node.attributes, HTMLName::kMethod);
  if (method_attr != nullptr &&
      strcasecmp(method_attr->value, kGetMethod) == 0) {
    form_method_is_post_ = false;
  }

  if (node.children.length > 0) {
    ParseFormFieldRecursive(node);
  }
  AnalyzeCurrentForm();

  requests_->emplace_back(current_request_.release());
}

void GumboFormFilter::ParseFormFieldRecursive(const GumboElement& element) {
  // Process the forms input fields.
  for (unsigned int i = 0; i < element.children.length; ++i) {
    const GumboNode* nested_node =
        static_cast<GumboNode*>(element.children.data[i]);
    if (nested_node->type != GUMBO_NODE_ELEMENT) {
      continue;
    }

    // TODO: Add support for more field types and store the value type
    // (string, int, bool, ..) together with the request parameter.
    if (nested_node->v.element.tag == GUMBO_TAG_INPUT ||
        nested_node->v.element.tag == GUMBO_TAG_TEXTAREA ||
        nested_node->v.element.tag == GUMBO_TAG_SELECT) {
      ProcessInputField(nested_node->v.element);
    }

    if (nested_node->v.element.children.length > 0) {
      ParseFormFieldRecursive(nested_node->v.element);
    }
  }
}

void GumboFormFilter::AddRequestParameter(const std::string& name,
                                          const std::string& value) {
  if (form_method_is_post_) {
    current_request_->SetPostParameter(name, value, false /* replace */);
  } else {
    current_request_->SetGetParameter(name, value, false /* replace */);
  }
}

void GumboFormFilter::ProcessInputField(const GumboElement& element) {
  std::map<std::string, std::string> attributes;
  GumboParser::FillAttributeMap(element, &attributes);
  if (attributes.count(HTMLName::kName) == 0) {
    DLOG(WARNING) << "Skipping element has without name attribute at line: "
                  << element.start_pos.line << ").";
    return;
  }

  if (attributes.count(HTMLName::kValue)) {
    AddRequestParameter(attributes[HTMLName::kName],
                        attributes[HTMLName::kValue]);
  } else {
    AddRequestParameter(attributes[HTMLName::kName], "");
  }

  // Return early if no type was found.
  if (attributes.count(HTMLName::kType) == 0) {
    return;
  }

  // Check if it's a password or XSRF field.
  const auto& type_value = attributes[HTMLName::kType];
  if (strcasecmp(type_value.c_str(), kPasswordFieldType) == 0) {
    form_has_password_field_ = true;
  }

  if (strcasecmp(type_value.c_str(), kHiddenFieldType) == 0 &&
      util::IsPotentialXSRFToken(attributes[HTMLName::kValue])) {
    DLOG(INFO) << "Found potential XSRF token. Name: "
               << attributes[HTMLName::kName];
    form_has_xsrf_token_ = true;
  }
}

}  // namespace plusfish
