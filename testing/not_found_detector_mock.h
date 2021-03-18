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

#ifndef PLUSFISH_TESTING_NOT_FOUND_DETECTOR_MOCK_H_
#define PLUSFISH_TESTING_NOT_FOUND_DETECTOR_MOCK_H_

#include "gmock/gmock.h"
#include "not_found_detector.h"

namespace plusfish {
namespace testing {

class MockNotFoundDetector : public NotFoundDetector {
 public:
  MOCK_METHOD(bool, AddUrl, (const std::string& url), (override));
};

}  // namespace testing
}  // namespace plusfish

#endif  // PLUSFISH_TESTING_NOT_FOUND_DETECTOR_MOCK_H_
