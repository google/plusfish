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

#include "audit/util/issue_util.h"

#include <memory>
#include <unordered_set>
#include "base/basictypes.h"

#include <glog/logging.h>
#include "absl/container/node_hash_set.h"

namespace plusfish {
namespace util {

// The amount of bytes to include include in a response snippets is based on the
// following approach: <100 bytes><payload offset><100 bytes>. This mean the
// snippet will never be longer than 200 bytes.
static const int kSnippetOffset = 100;

void UpdateIssueVectorWithSnippet(
    const IssueDetails::IssueType issue_type, const Severity severity,
    int64 req_id, const std::string& response_body, int64 response_body_offset,
    const std::string& extra_info,
    absl::node_hash_set<std::unique_ptr<IssueDetails>>* issues) {
  IssueDetails* issue = new IssueDetails();
  issue->set_severity(severity);
  issue->set_type(issue_type);
  issue->set_extra_info(extra_info);
  issue->set_request_id(req_id);
  issue->set_response_body_offset(response_body_offset);

  int64 snippet_size = kSnippetOffset;
  // Add a response snippet.
  if (response_body_offset < response_body.size()) {
    int64 start_offset = 0;
    if (kSnippetOffset < response_body_offset) {
      start_offset = response_body_offset - kSnippetOffset;
      snippet_size += kSnippetOffset;
    }

    issue->set_response_snippet(
        response_body.substr(start_offset, snippet_size));
  }

  issues->emplace(issue);
}

}  // namespace util
}  // namespace plusfish
