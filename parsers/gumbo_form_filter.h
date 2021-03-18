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

#ifndef OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_FORM_FILTER_H_
#define OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_FORM_FILTER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "absl/container/node_hash_set.h"
#include <gumbo.h>
#include "parsers/gumbo_filter.h"
#include "request.h"

namespace plusfish {

// This filter extracts forms HTML and saves them into Requests. These
// requests are stored in the Request vector that's given class upon
// initialisation. The caller will own the requests that are added to
// the requests vector.
//
// Example usage below:
//   vector<std::unique_ptr<Request>> requests_;
//   std::unordered_set<std::unique_ptr<IssueDetails>>
//   GumboFormFilter form_filter(original_request, &requests, &issues);
//   form_filter.ParseElement(gumbo_element);
class GumboFormFilter : public GumboFilter {
 public:
  GumboFormFilter();
  ~GumboFormFilter() override;

  // Initialize with an external vector to store the form requests. The base
  // request is responsible for the response that's being parsed by the filter.
  // Does not take ownership of the base_request.
  // The caller will own any Request added to the requests vector.
  GumboFormFilter(const Request* base_request,
                  std::vector<std::unique_ptr<Request>>* requests,
                  absl::node_hash_set<std::unique_ptr<IssueDetails>>* issues);

  // Used by the HTML parser to pass on Elements to the filter. The filter looks
  // for form related elements, such as <form> and <input>. These are parsed
  // into a (form) request.
  // Does not take ownership.
  void ParseElement(const GumboElement& node) override;

  // We do not extract forms from comments.
  void ParseComment(const GumboText& node) override {}
  // We do not extract forms from text nodes.
  void ParseText(const GumboText& node) override {}

 private:
  // Helper method to add a parameter to the existing form request.
  // Does not take ownership.
  void AddRequestParameter(const std::string& name, const std::string& value);

  // The given Element is a form field that has at least a 'name' attribute.
  // This element will be parsed into the form requests as either a POST or GET
  // parameter (depending on the form type).
  // Does not take ownership.
  void ProcessInputField(const GumboElement& element);

  // Recursively iterates over the form child nodes to parse out form
  // fields while ignoring other elements (e.g. nested divs).
  // Does not take ownership.
  void ParseFormFieldRecursive(const GumboElement& element);

  // Reviews the parsed form for vulnerabilities. Any detected issues will be
  // added to the issues set with which the class was initiated.
  void AnalyzeCurrentForm();

  // The current request where form fields are read into.
  std::unique_ptr<Request> current_request_;
  // The offset to the form in the response body.
  int64 form_response_body_offset_;
  // Indicates if the current form request uses POST.
  bool form_method_is_post_;
  // Indicates if the current form has a password field.
  bool form_has_password_field_;
  // Indicates if the current form has an XSRF token.
  bool form_has_xsrf_token_;
  // The request who's response we scrape for a form.
  const Request* base_request_;
  // A vector to store all collected form requests.
  // Not owned by this class.
  std::vector<std::unique_ptr<Request>>* requests_;
  // A set to store all collected issues.
  // Not owned by this class.
  absl::node_hash_set<std::unique_ptr<IssueDetails>>* issues_;
  DISALLOW_COPY_AND_ASSIGN(GumboFormFilter);
};

}  // namespace plusfish

#endif  // OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_FORM_FILTER_H_
