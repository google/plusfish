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

#ifndef OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_UTIL_ESCAPING_H_
#define OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_UTIL_ESCAPING_H_

#include <string>

namespace plusfish {
namespace util {
// Unescape HTML characters in the given string 'escaped' and return the
// results.
const std::string UnescapeHtml(const std::string& escaped);

// Returns a copy of the  given string with HTML characters escaped.
const std::string EscapeHtml(const std::string& unescaped);

}  // namespace util
}  // namespace plusfish

#endif  // OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_UTIL_ESCAPING_H_