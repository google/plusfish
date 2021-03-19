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

#ifndef OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_XSS_FILTER_H_
#define OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_XSS_FILTER_H_

#include <memory>
#include <string>
#include <vector>

#include "opensource/deps/base/macros.h"
#include "absl/container/node_hash_set.h"
#include <gumbo.h>
#include "parsers/gumbo_filter.h"
#include "proto/issue_details.pb.h"
#include "request.h"

namespace plusfish {

class Request;

// This HTML parser filter detects injected HTML tags and attributes. These
// detections are reported back via the issues set.
//
// Example usage:
//    std::unordered_set<std::unique_ptr<IssueDetails>> issues;
//    GumboXssFilter gumbo_xss_filter(original_request, &issues);
//    gumbo_xss_filter.ParseElement(GumboElement);
//    process(issues);
//    delete gumbo_xss_filter;
class GumboXssFilter : public GumboFilter {
 public:
  // Initialize with the request for which the response is parsed. The
  // externally initialized set is updated with any detected issue.
  // Does not take ownership.
  GumboXssFilter(const Request& orig_request,
                 absl::node_hash_set<std::unique_ptr<IssueDetails>>* issues);
  ~GumboXssFilter() override;

  // Parses the element to find injected XSS tags or injected attributes.
  void ParseElement(const GumboElement& node) override;
  // We currently don't search for XSS related strings in the HTML comments.
  void ParseComment(const GumboText& node) override {}
  // We don't search for XSS related strings in the text content of a page.
  void ParseText(const GumboText& node) override {}

 private:
  // The origin request.
  const Request& orig_request_;
  // All detected issues are added to this set.
  absl::node_hash_set<std::unique_ptr<IssueDetails>>* issues_;
  DISALLOW_COPY_AND_ASSIGN(GumboXssFilter);
};

}  //  namespace plusfish

#endif  // OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_XSS_FILTER_H_
