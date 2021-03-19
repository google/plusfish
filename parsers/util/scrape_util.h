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

#ifndef OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_UTIL_SCRAPE_UTIL_H_
#define OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_UTIL_SCRAPE_UTIL_H_

#include <memory>
#include <string>

#include "absl/container/node_hash_set.h"
#include "opensource/deps/base/integral_types.h"
#include "proto/http_common.pb.h"
#include "proto/issue_details.pb.h"

namespace plusfish {
namespace util {

// Parse JavaScript in a very simplistic way to scrape URLs. The parsing will
// capture pairs of keywords and quoted text. It then compares the keywords
// against the once we're interested in. If there is a match, the quoted text is
// returned in a string set. It works like this:
//
// The JavaScript 'document.location = "/hello";' is parsed as:
//   Keyword: document.location
//   Quoted text: /hello
//
// The parser only takes the first quoted string after a keyword. So for
// example, the JavaScript window.open('/hello', 'world'); is parsed as:
//   Keyword: window.open
//   Quoted text: /hello
//
// Which mean the second parameter is completely ignored.
// URLs need to have a trailing space to be grabbed without unrelated text.
bool ScrapeJs(const std::string& content,
              absl::node_hash_set<std::string>* anchors,
              absl::node_hash_set<std::unique_ptr<IssueDetails>>* issues);

// A generic URL scraper which uses a regular expression to extract URLs from
// the content. The URLs are returned in a set of unique strings.
// URLs scraped by this function are not deduplicated.
absl::node_hash_set<std::string> ScrapeUrl(const std::string& content);

// Suffix comparison helper method.
bool JsKeywordSuffixMatch(const std::string& keyword);

// Match the given string against the XSS payload regex and extract the request
// ID into the request_id. Returns true on success and false on failure.
// Upon failure, request_id is also not set.
bool MatchXssPayload(const std::string& value, int64* request_id);

// Search the given string for the XSS payload regex and extract the request
// ID into the request_id. Returns true on success and false on failure.
// Upon failure, request_id is also not set.
bool SearchXssTagPayload(const std::string& value, int64* request_id);

// Tries to determine the content type by simulating browser content sniffing.
// Will update the given mime with the sniffed mime when it's found to be
// different. Returns true if a mime was sniffed, else false.
bool SniffMimeType(const std::string& content, MimeInfo::MimeType* mime);

// Tests if the given value is a potential XSRF token. If the string is
// base 10/16/32 or 64 and with the right length true will be returned. Too
// long values, especially base 64, are likely used for non-XSRF purposes
// (e.g. encoded JSON) which is why we have length limits.
bool IsPotentialXSRFToken(const std::string& value);

}  // namespace util
}  // namespace plusfish

#endif  // OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_UTIL_SCRAPE_UTIL_H_
