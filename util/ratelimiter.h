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

#ifndef PLUSFISH_UTIL_RATELIMITER_H_
#define PLUSFISH_UTIL_RATELIMITER_H_

#include "opensource/deps/base/macros.h"

namespace plusfish {

class RateLimiterInterface {
 public:
  explicit RateLimiterInterface(const int rate_per_sec);
  virtual ~RateLimiterInterface() {}

  // Returns the number of slots used in the rate limiter period.
  virtual const int used_in_period() = 0;

  // Returns the maximum request rate.
  virtual const int max_rate() const = 0;

  // Returns true if the caller can perform the rate limited action. False is
  // returned if the rate is already exceeded.
  virtual bool TakeRateSlot() = 0;

  // Returns true if the caller can do the rate limited action. False is
  // returned if the rate is already exceeded. The caller has to give the
  // current time as a parameter.
  virtual bool TakeRateSlotWithTime(int64 current_time_sec) = 0;

 protected:
  RateLimiterInterface() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(RateLimiterInterface);
};

}  // namespace plusfish

#endif  // PLUSFISH_UTIL_RATELIMITER_H_
