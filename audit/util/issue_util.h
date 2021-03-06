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

#ifndef OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_AUDIT_UTIL_ISSUE_UTIL_H_
#define OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_AUDIT_UTIL_ISSUE_UTIL_H_

#include <memory>
#include <unordered_set>

#include "absl/container/node_hash_set.h"
#include "opensource/deps/base/basictypes.h"
#include "proto/issue_details.pb.h"

namespace plusfish {
namespace util {

// Creates an IssueDetails instance with the given parameters and adds it to the
// issues vector. A response snippet will be created using the payload offset.
// Does not take ownership.
void UpdateIssueVectorWithSnippet(
    const IssueDetails::IssueType issue_type, const Severity severity,
    int64 req_id, const std::string& response_body, int64 response_body_offset,
    const std::string& extra_info,
    absl::node_hash_set<std::unique_ptr<IssueDetails>>* issues);

}  // namespace util
}  // namespace plusfish

#endif  // OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_AUDIT_UTIL_ISSUE_UTIL_H_
