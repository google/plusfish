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

#include <ctime>

#include "base/integral_types.h"
#include <glog/logging.h>
#include "absl/synchronization/mutex.h"

namespace plusfish {

// The period, in seconds, over which the rate is limited.
static const int kRatePeriod = 1;

SimpleRateLimiter::SimpleRateLimiter(const int rate_per_sec)
    : max_rate_(rate_per_sec), used_in_period_(0), period_end_(0) {
  if (rate_per_sec <= 0) {
    LOG(WARNING) << "Received unrealistic rate: " << rate_per_sec
                 << " defaulting to 1 (really slow!)";
    max_rate_ = 1;
  }
}

SimpleRateLimiter::~SimpleRateLimiter() {}

bool SimpleRateLimiter::TakeRateSlot() {
  return TakeRateSlotWithTime(time(nullptr));
}

bool SimpleRateLimiter::TakeRateSlotWithTime(int64 current_time_sec) {
  absl::MutexLock l(&used_in_period_mutex_);
  if (current_time_sec >= period_end_) {
    period_end_ = current_time_sec + kRatePeriod;
    used_in_period_ = 1;
    return true;
  }

  if (used_in_period_ < max_rate_) {
    ++used_in_period_;
    return true;
  }
  return false;
}

}  // namespace plusfish
