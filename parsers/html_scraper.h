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

#ifndef OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_HTML_SCRAPER_H_
#define OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_HTML_SCRAPER_H_

#include <memory>
#include <vector>

#include "opensource/deps/base/macros.h"
#include "absl/container/node_hash_set.h"
#include "parsers/gumbo_parser.h"
#include "parsers/html_parser.h"
#include "proto/issue_details.pb.h"
#include "request.h"
#include "util/html_fingerprint.h"

namespace plusfish {

// Parses HTML using the gumbo parser. The scraper collect anchors and security
// issues using GumboFilters which each are given document nodes while iterating
// over the parsed HTML tree.
// Sample usage:
//    HtmlScraper scraper;
//    if (scraper.Parse("http://example.org", http_response_string)) {
//      process(scraper.anchors());
//    }
//    delete scraper;
class HtmlScraper : HtmlParserInterface {
 public:
  HtmlScraper();
  ~HtmlScraper() override;

  // Returns a vector with collected URLs.
  const std::vector<std::string>& anchors() const override { return anchors_; }

  // Returns a vector with collected Requests.
  std::vector<std::unique_ptr<Request>>& requests() override {
    return requests_;
  }

  // Returns a set with collected issues.
  absl::node_hash_set<std::unique_ptr<IssueDetails>>& issues() override {
    return issues_;
  }

  // Return the HTML fingerprint
  std::unique_ptr<HtmlFingerprint> fingerprint() {
    return std::move(fingerprint_);
  }

  // Parse HTML content. The Request is used for parsing relative URLs
  // and registering security issues. Returns true when the Request and
  // content were processed successfully. Else false is returned.
  bool Parse(const Request* request, const std::string& html_content) override;

 private:
  // Anchors scraped from the response.
  std::vector<std::string> anchors_;
  // Requests (e.g. forms) scraped from the response.
  std::vector<std::unique_ptr<Request>> requests_;
  // Issues returned by filters.
  absl::node_hash_set<std::unique_ptr<IssueDetails>> issues_;
  // HTML fingerprint of the request.
  std::unique_ptr<HtmlFingerprint> fingerprint_;
  std::unique_ptr<GumboParser> gumbo_parser_;

  DISALLOW_COPY_AND_ASSIGN(HtmlScraper);
};

}  // namespace plusfish

#endif  // OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_HTML_SCRAPER_H_
