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

#ifndef PLUSFISH_TESTING_HTTP_CLIENT_MOCK_H_
#define PLUSFISH_TESTING_HTTP_CLIENT_MOCK_H_

#include "gmock/gmock.h"
#include "http_client.h"

namespace plusfish {
namespace testing {

// Mock for the HTTP client class.
class MockHttpClient : public HttpClientInterface {
 public:
  MOCK_METHOD0(schedule_queue_size, size_t());
  MOCK_METHOD0(active_requests_count, size_t());
  MOCK_METHOD0(requests_performed_count, size_t());
  MOCK_METHOD1(Schedule, bool(Request* req));
  MOCK_METHOD2(Schedule, bool(Request* req, RequestHandlerInterface* rh));
  MOCK_METHOD3(RegisterDefaultHeader,
               bool(const std::string& domain, const std::string& name,
                    const std::string& value));
  MOCK_METHOD0(StartNewRequests, bool());
  MOCK_METHOD0(Poll, int());
  MOCK_METHOD0(Enable, void());
  MOCK_METHOD0(Disable, void());
  MOCK_METHOD0(enabled, bool());
};

}  // namespace testing
}  // namespace plusfish

#endif  // PLUSFISH_TESTING_HTTP_CLIENT_MOCK_H_
