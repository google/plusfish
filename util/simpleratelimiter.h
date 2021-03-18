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

#ifndef PLUSFISH_UTIL_SIMPLERATELIMITER_H_
#define PLUSFISH_UTIL_SIMPLERATELIMITER_H_

#include "base/integral_types.h"
#include "base/macros.h"
#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"
#include "util/ratelimiter.h"

namespace plusfish {

// A simple rate limiter that prevents the rate of requests from being exceeded.
// It does not prevent the limit from being reached in the first 0.0001 second
// of the second though (bursts).
//
// Based on talk/base/ratelimiter.h
class SimpleRateLimiter : public RateLimiterInterface {
 public:
  explicit SimpleRateLimiter(const int rate_per_sec);
  ~SimpleRateLimiter() override;

  const int used_in_period() override {
    absl::ReaderMutexLock r(&used_in_period_mutex_);
    return used_in_period_;
  }

  const int max_rate() const override { return max_rate_; }

  // Returns true if the caller can perform the rate limited action. False is
  // returned if the rate is already exceeded.
  bool TakeRateSlot() override;

  // Returns true if the caller can do the rate limited action. False is
  // returned if the rate is already exceeded. The caller has to give the
  // current time as a parameter.
  bool TakeRateSlotWithTime(int64 current_time_sec) override;

 private:
  int max_rate_;
  int used_in_period_ ABSL_GUARDED_BY(used_in_period_mutex_);
  int64 period_end_;
  absl::Mutex used_in_period_mutex_;
  DISALLOW_COPY_AND_ASSIGN(SimpleRateLimiter);
};

}  // namespace plusfish

#endif  // PLUSFISH_UTIL_SIMPLERATELIMITER_H_
