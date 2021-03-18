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

#include "audit/util/audit_util.h"

#include <iomanip>
#include <set>
#include <sstream>
#include <string>

#include "parsers/util/escaping.h"
#include "proto/generator.pb.h"

namespace plusfish {
namespace util {

// Characters that are ignored when encoding payloads for URLs.
static const std::set<char> kUrlEncodeIgnoreChars = {'&', '=', ';', ',',
                                                     '!', '$', '?', '%'};

std::string encode_url_token_partial(const std::string& token,
                                     const std::set<char>& ignore) {
  std::ostringstream encoded;
  for (const char c : token) {
    if (isalnum(c) == 0 && ignore.count(c) == 0) {
      encoded << "%" << std::hex << static_cast<unsigned int>(c);
    } else {
      encoded << c;
    }
  }
  return encoded.str();
}

std::string encode_url_token(const std::string& token) {
  return encode_url_token_partial(token, kUrlEncodeIgnoreChars);
}

std::string GeneratePayloadString(const std::string& original_value,
                                  const std::string& payload,
                                  const GeneratorRule_InjectionMethod method,
                                  const GeneratorRule_EncodingType encoding) {
  std::string final_payload;
  switch (method) {
    case GeneratorRule_InjectionMethod_APPEND_TO_VALUE:
      final_payload.append(original_value + payload);
      break;
    case GeneratorRule_InjectionMethod_PREFIX_VALUE:
      final_payload.append(payload + original_value);
      break;
    case GeneratorRule_InjectionMethod_SET_VALUE:
      // Pass through: this is the default.
    default:
      final_payload = payload;
  }

  switch (encoding) {
    case GeneratorRule_EncodingType_URL:
      return encode_url_token(final_payload);
    case GeneratorRule_EncodingType_HTML:
      return util::EscapeHtml(final_payload);
    case GeneratorRule_EncodingType_NONE:
      // Pass through: this is the default.
    default:
      return final_payload;
  }
}

}  // namespace util
}  // namespace plusfish
