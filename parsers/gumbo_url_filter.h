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

#ifndef OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_URL_FILTER_H_
#define OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_URL_FILTER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "absl/container/node_hash_set.h"
#include <gumbo.h>
#include "parsers/gumbo_filter.h"
#include "proto/issue_details.pb.h"

namespace plusfish {

// This gumbo filter extracts URLs from HTML elements. The URLs are stored
// in the vector that's provided by the caller upon creating an instance.
//
// Example usage:
//    vector<string> anchors;
//    std::unordered_set<std::unique_ptr<IssueDetails>> issues;
//    GumboUrlFilter url_filter(&anchors, &issues);
//    url_filter.ParseElement(GumboElement);
//    html_parser.FinishParse();
//    process(anchors, issues);
//    delete url_filter;
class GumboUrlFilter : public GumboFilter {
 public:
  // Initialize with an external vector to store the anchors and an issues set
  // for storing any detected issues.
  // Does not take ownership.
  GumboUrlFilter(std::vector<std::string>* anchors,
                 absl::node_hash_set<std::unique_ptr<IssueDetails>>* issues);
  ~GumboUrlFilter() override;

  // Parses URLs from a gumbo element node.
  void ParseElement(const GumboElement& node) override;
  // Parses URLs from HTML comment sections.
  void ParseComment(const GumboText& node) override;
  // Parses URLs from text content.
  void ParseText(const GumboText& node) override;

 private:
  // Method to extract the URL from a meta element.
  // Returns true when the element contained a URL (which was extracted).
  bool ParseMeta(const GumboElement& element);

  // Helper method that adds a new anchor to the anchors vector. Unless
  // it's already present. If the anchor starts with javascript: the content
  // will be parsed as JavaScript.
  void AddAnchor(const std::string& new_anchor);

  // Same as AddAnchor except that it adds all anchors from a set.
  void AddAnchorsSet(const absl::node_hash_set<std::string>& anchors);

  // Same as AddAnchorsSet except that the anchors are HTML unescaped before
  // being added to the list.
  void AddAnchorsSetHtmlUnescaped(
      const absl::node_hash_set<std::string>& anchors);

  // Last Gumbo element handled by ParseElement.
  const GumboElement* last_element_;
  // A unique collection of anchors.
  std::vector<std::string>* anchors_;
  // A pointer to the set of issues which was given to the class instance during
  // creation time. All detected issues are added to this vector.
  absl::node_hash_set<std::unique_ptr<IssueDetails>>* issues_;
  DISALLOW_COPY_AND_ASSIGN(GumboUrlFilter);
};

}  //  namespace plusfish

#endif  // OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_URL_FILTER_H_
