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

#include "util/simpleratelimiter.h"

#include "gtest/gtest.h"

namespace plusfish {

TEST(RatelimiterTest, LimitsOk) {
  int fake_time = 3;
  SimpleRateLimiter limiter(2);
  EXPECT_TRUE(limiter.TakeRateSlotWithTime(fake_time));
  EXPECT_TRUE(limiter.TakeRateSlotWithTime(fake_time));
  EXPECT_FALSE(limiter.TakeRateSlotWithTime(fake_time));
  EXPECT_EQ(2, limiter.used_in_period());
  EXPECT_EQ(2, limiter.max_rate());
}

TEST(RatelimiterTest, LimitRecentsOnNewTime) {
  int fake_time = 3;
  SimpleRateLimiter limiter(2);
  EXPECT_TRUE(limiter.TakeRateSlotWithTime(fake_time));
  EXPECT_TRUE(limiter.TakeRateSlotWithTime(fake_time));
  EXPECT_FALSE(limiter.TakeRateSlotWithTime(fake_time));
  EXPECT_EQ(2, limiter.used_in_period());
  EXPECT_TRUE(limiter.TakeRateSlotWithTime(fake_time + 1));
  EXPECT_EQ(1, limiter.used_in_period());
}

TEST(RatelimiterTest, LimitsWeirdRateOk) {
  int fake_time = 3;
  SimpleRateLimiter limiter(-1);
  EXPECT_TRUE(limiter.TakeRateSlotWithTime(fake_time));
  EXPECT_FALSE(limiter.TakeRateSlotWithTime(fake_time));
}

}  // namespace plusfish
