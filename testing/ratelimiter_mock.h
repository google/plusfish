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

#ifndef PLUSFISH_TESTING_RATELIMITER_MOCK_H_
#define PLUSFISH_TESTING_RATELIMITER_MOCK_H_

#include "gmock/gmock.h"
#include "util/ratelimiter.h"

namespace plusfish {
namespace testing {

class MockRateLimiter : public RateLimiterInterface {
 public:
  MOCK_METHOD0(used_in_period, const int());
  MOCK_CONST_METHOD0(max_rate, const int());
  MOCK_METHOD0(TakeRateSlot, bool());
  MOCK_METHOD1(TakeRateSlotWithTime, bool(int64 current_time_sec));
};

}  // namespace testing
}  // namespace plusfish

#endif  // PLUSFISH_TESTING_RATELIMITER_MOCK_H_
