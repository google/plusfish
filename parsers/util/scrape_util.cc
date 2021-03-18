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

#include "parsers/util/scrape_util.h"

#include <string.h>

#include <cctype>
#include <memory>
#include <string>
#include <unordered_set>

#include <glog/logging.h>
#include "absl/container/node_hash_set.h"
#include "re2/re2.h"

// The regex to match plusfish XSS payloads. The expected format is:
// plush<request_id>fish.
static LazyRE2 kXssPayloadRegex = {"(?i)plus([0-9]+)fish"};
static LazyRE2 kTagXssPayloadRegex = {"(?i)<plus([0-9]+)fish>"};

// The maximum amount of bytes we want to use for content sniffing.
static int kContentBufferSize = 1024;

namespace {
// Calculate the size of a quoted string. The first quote is expected to be
// present at the given offset in the content string. The returned size
// does not include the quote(s).
int GetQuotedTextSize(const std::string& content, int offset) {
  char quote = content[offset];
  for (int char_index = 1; char_index < content.size() - offset; ++char_index) {
    // Skip escaped characters.
    if (content[offset + char_index] == '\\') {
      ++char_index;
      continue;
    }
    // The end quote.
    if (content[offset + char_index] == quote) {
      return char_index - 1;
    }
  }
  return content.size() - offset;
}
}  // namespace

namespace plusfish {
namespace util {

// The regex used to grab (potential) absolute URLs.
static LazyRE2 kAbsoluteUrlRegex = {
    "((https?:)?//[a-zA-Z0-9-]+\\.?[a-zA-Z0-9-]+[^\\s]*)"};

// The regex used to grab (potential) relative URLs. This expects the relative
// URL to have a ../<word> or /word prefix.
static LazyRE2 kRelativeUrlRegex = {
    "(^(\\/?\\.\\.\\/|\\/)[\\w*\\.\\/]+[^\\s]*)"};

// Keywords from which we want to capture the quoted text.
static const std::vector<const char*> kJsKeywordSuffixes = {
    ".href", ".src", ".action", "location", "location.assign", "window.open"};

// XSRF detection constants which define the character composition, minimum and
// maximum string lengths.
static const int kXsrfBase16MinLength = 8;     // Minimum base10/16 token length
static const int kXsrfBase16MaxLength = 45;    // Maximum base10/16 token length
static const int kXsrfBase16MinDigits = 2;     // ...minimum digit count
static const int kXsrfBase64MinLength = 6;     // Minimum base32/64 token length
static const int kXsrfBase64MaxLength = 52;    // Maximum base32/64 token length
static const int kXsrfBase64MinDigits = 1;     // ...minimum digit count &&
static const int kXsrfBase64MinUpperCase = 2;  // ...minimum uppercase count
static const int kXsrfBase64AltMinDigits = 3;  // ...digit count override
static const int kXsrfBase64MaxSlash = 2;      // ...maximum slash count
// Characters that are acceptable base 10/16 tokens.
static const char* const kBase16Chars = "abcdef";
// Non-alphanumeric characters that are acceptable in base64 tokens.
static const char* const kBase64Chars = "=+/";
// TODO: move the characters above to an unordered_set with lambda
// initialization.

bool IsPotentialBase16Token(const std::string& value) {
  int digit_count = 0;
  int current_offset = 0;

  for (const char& value_char : value) {
    if (isdigit(value_char)) {
      ++digit_count;
    } else if (!strchr(kBase16Chars, tolower(value_char))) {
      break;
    }

    // Increate offset and bail out when we already exceed the max size.
    if (++current_offset > kXsrfBase16MaxLength) {
      return false;
    }
  }
  return (current_offset >= kXsrfBase16MinLength &&
          digit_count >= kXsrfBase16MinDigits);
}

bool IsPotentialBase64Token(const std::string& value) {
  int slash_count = 0;
  int upper_count = 0;
  int digit_count = 0;
  int current_offset = 0;

  for (const char& value_char : value) {
    if (!isalnum(value_char) && !strchr(kBase64Chars, value_char)) {
      break;
    }
    if (isupper(value_char)) {
      ++upper_count;
    } else if (isdigit(value_char)) {
      ++digit_count;
    } else if (value_char == '/') {
      ++slash_count;
    }

    // Increate offset and bail out when we already exceed the max size.
    if (++current_offset > kXsrfBase64MaxLength) {
      return false;
    }
  }
  return (current_offset >= kXsrfBase64MinLength &&
          slash_count <= kXsrfBase64MaxSlash &&
          (digit_count >= kXsrfBase64AltMinDigits ||
           (digit_count >= kXsrfBase64MinDigits &&
            upper_count >= kXsrfBase64MinUpperCase)));
}

bool IsPotentialXSRFToken(const std::string& value) {
  return IsPotentialBase16Token(value) || IsPotentialBase64Token(value);
}

bool MatchXssPayload(const std::string& value, int64* request_id) {
  return RE2::FullMatch(value, *kXssPayloadRegex, request_id);
}

bool SearchXssTagPayload(const std::string& value, int64* request_id) {
  return RE2::PartialMatch(value, *kTagXssPayloadRegex, request_id);
}

bool ScrapeJs(const std::string& content,
              absl::node_hash_set<std::string>* anchors,
              absl::node_hash_set<std::unique_ptr<IssueDetails>>* issues) {
  int last_keyword_start = 0;
  std::string last_keyword = "";

  // Split the response in lines.
  for (int char_index = 0; char_index < content.size(); ++char_index) {
    //  Handle quoted strings.
    if (content[char_index] == '\'' || content[char_index] == '"') {
      int quoted_text_size = GetQuotedTextSize(content, char_index);
      if (!last_keyword.empty() && quoted_text_size != 0) {
        // Grab URLs that are given as arguments to JavaScript methods that are
        // known to take URLs (e.g. location.href).
        std::string quoted_text =
            content.substr(char_index + 1, quoted_text_size);
        if (JsKeywordSuffixMatch(last_keyword) && !quoted_text.empty()) {
          anchors->insert(quoted_text);
          DLOG(INFO) << "Added: " << quoted_text
                     << " (keyword: " << last_keyword << ")";
          // Only capture the first quoted text.
          last_keyword.clear();
        } else {
          // Detect URLs in quoted strings.
          if (RE2::FullMatch(quoted_text, *kRelativeUrlRegex) ||
              RE2::FullMatch(quoted_text, *kAbsoluteUrlRegex)) {
            anchors->insert(quoted_text);
          }
        }
      }
      char_index += quoted_text_size + 1;
      continue;
    }

    if (isalnum(content[char_index]) || content[char_index] == '.') {
      if (last_keyword_start == -1) {
        last_keyword_start = char_index;
      }
    } else if (last_keyword_start != -1) {
      last_keyword =
          content.substr(last_keyword_start, char_index - last_keyword_start);
      last_keyword_start = -1;
      // Check if the injected keyword is a plusfish XSS payload.
      int64 req_id;
      if (MatchXssPayload(last_keyword, &req_id)) {
        DLOG(INFO) << "Found XSS in javascript context: " << last_keyword;
        IssueDetails* issue = new IssueDetails();
        issue->set_severity(Severity::HIGH);
        issue->set_type(IssueDetails::XSS_REFLECTED_JAVASCRIPT);
        issue->set_issue_name("Reflected XSS");
        issue->set_extra_info("Found payload in javascript context");
        issue->set_request_id(req_id);
        issues->emplace(std::unique_ptr<IssueDetails>(issue));
      }
    }
  }
  return true;
}

absl::node_hash_set<std::string> ScrapeUrl(const std::string& content) {
  std::string anchor;
  absl::node_hash_set<std::string> anchors;
  re2::StringPiece tmp_content(content);
  while (RE2::FindAndConsume(&tmp_content, *kAbsoluteUrlRegex, &anchor)) {
    anchors.insert(anchor);
  }
  return anchors;
}

// This test is here to see if one of the vector values is a suffix match for
// the keyword (e.g. keyword = "foobar.href" and a vector value is ".href").
bool JsKeywordSuffixMatch(const std::string& keyword) {
  for (const std::string& suffix : kJsKeywordSuffixes) {
    // No point in matching a partial string that's bigger than the keyword.
    if (suffix.length() > keyword.length()) {
      continue;
    }

    if (keyword.compare(keyword.length() - suffix.length(), suffix.length(),
                        suffix) == 0) {
      return true;
    }
  }
  return false;
}

bool SniffMimeType(const std::string& content, MimeInfo::MimeType* mime) {
  std::string temp;
  const char* sniffbuf;
  // Grab a substring if the content is too large.
  if (content.length() > kContentBufferSize) {
    temp = content.substr(0, kContentBufferSize);
    sniffbuf = temp.c_str();
  } else {
    sniffbuf = content.c_str();
  }

  // Detect xml/ types.
  if (strcasestr(sniffbuf, "<?xml ")) {
    if (strcasestr(sniffbuf, "<!DOCTYPE html") ||
        strcasestr(sniffbuf, "http://www.w3.org/1999/xhtml")) {
      *mime = MimeInfo::XML_XHTML;
      return true;
    }
    *mime = MimeInfo::XML_GENERIC;
    return true;
  }

  // Detect text/html.
  if (strcasestr(sniffbuf, "<!DOCTYPE html") || strcasestr(sniffbuf, "<html") ||
      strcasestr(sniffbuf, "<title") || strcasestr(sniffbuf, "<head>") ||
      strcasestr(sniffbuf, "<body") || strcasestr(sniffbuf, "<table") ||
      strcasestr(sniffbuf, "<form") || strcasestr(sniffbuf, "<p>") ||
      strcasestr(sniffbuf, "<foo")) {
    *mime = MimeInfo::ASC_HTML;
    return true;
  }
  return false;
}

}  // namespace util
}  // namespace plusfish
