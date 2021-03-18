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

#include "parsers/gumbo_xss_filter.h"

#include <memory>
#include <string>
#include <vector>

#include "absl/container/node_hash_set.h"
#include <gumbo.h>
#include "audit/util/issue_util.h"
#include "parsers/util/scrape_util.h"
#include "request.h"

namespace plusfish {

GumboXssFilter::GumboXssFilter(
    const Request& orig_request,
    absl::node_hash_set<std::unique_ptr<IssueDetails>>* issues)
    : orig_request_(orig_request), issues_(issues) {}

GumboXssFilter::~GumboXssFilter() {}

void GumboXssFilter::ParseElement(const GumboElement& node) {
  int64 req_id = 0;

  if (node.tag == GUMBO_TAG_UNKNOWN) {
    // We don't perform a full match on unknown tags because of a limitation in
    // Gumbo where it sometimes includes additional text from the page in the
    // tag.
    if (util::SearchXssTagPayload(node.original_tag.data, &req_id)) {
      util::UpdateIssueVectorWithSnippet(
          IssueDetails::XSS_REFLECTED_TAG, Severity::HIGH, req_id,
          orig_request_.response()->body(), node.start_pos.offset,
          "Found injected XSS payload at line: " +
              std::to_string(node.start_pos.line),
          issues_);
    }
    return;
  }

  for (int i = 0; i < node.attributes.length; i++) {
    const auto& attr = *static_cast<GumboAttribute*>(node.attributes.data[i]);
    if (util::MatchXssPayload(attr.name, &req_id)) {
      util::UpdateIssueVectorWithSnippet(
          IssueDetails::XSS_REFLECTED_ATTRIBUTE, Severity::HIGH, req_id,
          orig_request_.response()->body(), node.start_pos.offset,
          "Found injected XSS payload at line: " +
              std::to_string(node.start_pos.line),
          issues_);
      return;
    }
  }
}

}  //  namespace plusfish
