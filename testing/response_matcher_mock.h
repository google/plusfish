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

#ifndef PLUSFISH_TESTING_RESPONSE_MATCHER_MOCK_H_
#define PLUSFISH_TESTING_RESPONSE_MATCHER_MOCK_H_

#include "audit/response_matcher.h"

#include <memory>
#include <vector>

#include "gmock/gmock.h"

namespace plusfish {
class Request;

namespace testing {
class MockResponseMatcher : public ResponseMatcherInterface {
 public:
  MOCK_METHOD0(Init, bool());
  MOCK_METHOD1(Match,
               bool(const std::vector<std::unique_ptr<Request>>* requests));
  MOCK_METHOD1(MatchSingle, bool(const Request* request));
};

}  // namespace testing
}  // namespace plusfish
#endif  // PLUSFISH_TESTING_RESPONSE_MATCHER_MOCK_H_
