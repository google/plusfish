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

#ifndef PLUSFISH_TESTING_REQUEST_MOCK_H_
#define PLUSFISH_TESTING_REQUEST_MOCK_H_

#include "gmock/gmock.h"
#include "request.h"
#include "response.h"
#include "util/html_fingerprint.h"

namespace plusfish {
namespace testing {

class MockRequest : public Request {
 public:
  explicit MockRequest(const std::string& url) : Request(url) {}
  MOCK_METHOD0(DoneCb, void());
  MOCK_CONST_METHOD0(response, const Response*());
  MOCK_CONST_METHOD0(url, const std::string&());
  MOCK_METHOD0(truncate_response_body, void());
  MOCK_CONST_METHOD0(parent_id, const int64());
  MOCK_METHOD1(set_response_html_fingerprint,
               void(std::unique_ptr<HtmlFingerprint> fingerprint));

 private:
  // Disallow mock requests to be created without a std::string url because we
  // use it heavily in DLOG statements which then fail in tests.
  MockRequest() {}
};

}  // namespace testing
}  // namespace plusfish

#endif  // PLUSFISH_TESTING_REQUEST_MOCK_H_
