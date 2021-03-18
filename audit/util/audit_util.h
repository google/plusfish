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

#ifndef PLUSFISH_AUDIT_UTIL_AUDIT_UTIL_H_
#define PLUSFISH_AUDIT_UTIL_AUDIT_UTIL_H_

#include <set>
#include <string>

#include "proto/generator.pb.h"

namespace plusfish {
namespace util {

// Returns a std::string where all non-alphanumeric are percent encoded.
std::string encode_url_token(const std::string& token);

// Returns a std::string where all non-alphanumeric characters, not present
// in as a character in the ignore parameter, are percent encoded.
std::string encode_url_token_partial(const std::string& token,
                                     const std::set<char>& ignore);

// Generates a payload std::string based on the current value, injection method
// and desired encoding. The encoding is applied to the entire payload
// std::string (after modification of the original value).
std::string GeneratePayloadString(const std::string& original_value,
                                  const std::string& payload,
                                  const GeneratorRule_InjectionMethod method,
                                  const GeneratorRule_EncodingType encoding);

}  // namespace util
}  // namespace plusfish
#endif  // PLUSFISH_AUDIT_UTIL_AUDIT_UTIL_H_
