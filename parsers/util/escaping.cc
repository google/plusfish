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

#include "parsers//util/escaping.h"

#include <map>
#include <string>

#include <glog/logging.h>

namespace plusfish {
namespace util {

// Helper method to unescape HTML characters from a string.
// Returns an unescaped copy of the string.
const std::string UnescapeHtml(const std::string& escaped) {
  std::string ret = escaped;
  std::map<std::string, std::string> chars = {
      {"&amp;", "&"},  {"&#38;", "&"}, {"&lt;", "<"},    {"&#60;", "<"},
      {"&gt;", ">"},   {"&#62;", ">"}, {"&quot;", "\""}, {"&#34;", "\""},
      {"&apos;", "'"}, {"&#39;", "'"},

  };
  size_t found;
  for (const auto& entry : chars) {
    do {
      found = ret.find(entry.first);
      if (found != std::string::npos) {
        ret.replace(found, entry.first.length(), entry.second);
      }
    } while (found != std::string::npos);
  }

  return ret;
}

// Helper method to escape special characters for use in HTML.
// Returns an HTML escaped copy of the string.
const std::string EscapeHtml(const std::string& unescaped) {
  std::string return_buffer;
  return_buffer.reserve(unescaped.size() * 1.2L);
  for (size_t pos = 0; pos != unescaped.size(); ++pos) {
    switch (unescaped[pos]) {
      case '<':
        return_buffer.append("&lt;");
        break;
      case '>':
        return_buffer.append("&gt;");
        break;
      case '&':
        return_buffer.append("&amp;");
        break;
      case '\"':
        return_buffer.append("&quot;");
        break;
      case '\'':
        return_buffer.append("&apos;");
        break;
      default:
        return_buffer.append(&unescaped[pos], 1);
        break;
    }
  }
  return return_buffer;
}

}  // namespace util
}  // namespace plusfish
