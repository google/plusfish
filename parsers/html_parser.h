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

#ifndef OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_HTML_PARSER_H_
#define OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_HTML_PARSER_H_

#include <string>
#include <unordered_set>
#include <vector>

#include "opensource/deps/base/macros.h"
#include "absl/container/node_hash_set.h"
#include "request.h"

namespace plusfish {

class IssueDetails;

// Generic HTML parser interface for plusfish.
class HtmlParserInterface {
 public:
  virtual ~HtmlParserInterface() {}

  // Parse the HTML content in the string.
  virtual bool Parse(const Request* request,
                     const std::string& html_content) = 0;
  // Return the anchors collected by the parser.
  virtual const std::vector<std::string>& anchors() const = 0;
  // Return the Requests collected by the parser.
  virtual std::vector<std::unique_ptr<Request>>& requests() = 0;
  // Return the IssueDetails collected by the parser.
  virtual absl::node_hash_set<std::unique_ptr<IssueDetails>>& issues() = 0;

 protected:
  HtmlParserInterface() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(HtmlParserInterface);
};

}  // namespace plusfish

#endif  // OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_HTML_PARSER_H_
