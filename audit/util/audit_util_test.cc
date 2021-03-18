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

#include "gtest/gtest.h"
#include "proto/generator.pb.h"

namespace plusfish {
namespace util {

TEST(AuditUtilTest, AppendsPayloadWithoutEncoding) {
  std::string current_value = "foo";
  std::string payload = "bar";
  std::string expected_final_result = current_value + payload;
  std::string final_payload = GeneratePayloadString(
      current_value, payload, GeneratorRule_InjectionMethod_APPEND_TO_VALUE,
      GeneratorRule_EncodingType_NONE);

  ASSERT_STREQ(final_payload.c_str(), expected_final_result.c_str());
}

TEST(AuditUtilTest, ReplacesValueWithoutEncoding) {
  std::string current_value = "yes";
  std::string payload = "no";
  std::string final_payload = GeneratePayloadString(
      current_value, payload, GeneratorRule_InjectionMethod_SET_VALUE,
      GeneratorRule_EncodingType_NONE);

  ASSERT_STREQ(final_payload.c_str(), payload.c_str());
}

TEST(AuditUtilTest, ReplacesValueAndEncodeHtml) {
  std::string current_value = "yes";
  std::string payload = "<script>";
  std::string final_payload = GeneratePayloadString(
      current_value, payload, GeneratorRule_InjectionMethod_SET_VALUE,
      GeneratorRule_EncodingType_HTML);
  ASSERT_STREQ("&lt;script&gt;", final_payload.c_str());
}

TEST(AuditUtilTest, ReplacesValueAndEncodeUrl) {
  std::string current_value = "yes";
  std::string payload = "/script/";
  std::string final_payload = GeneratePayloadString(
      current_value, payload, GeneratorRule_InjectionMethod_SET_VALUE,
      GeneratorRule_EncodingType_URL);
  ASSERT_STREQ("%2fscript%2f", final_payload.c_str());
}

TEST(AuditUtilTest, EncodeUrlCharsOK) {
  std::string token_to_encode("../etc/passwd");
  ASSERT_STREQ("%2e%2e%2fetc%2fpasswd",
               encode_url_token(token_to_encode).c_str());
}

TEST(AuditUtilTest, EncodeUrlCharsSkipsAsExpected) {
  std::string token_to_encode("please;skip&do;not;encode;the;%and&chars");
  ASSERT_STREQ(token_to_encode.c_str(),
               encode_url_token(token_to_encode).c_str());
}
}  // namespace util
}  // namespace plusfish
