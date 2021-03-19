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

#ifndef THIRD_PARTY_PLUSFISH_UTIL_CLOCK_H_
#define THIRD_PARTY_PLUSFISH_UTIL_CLOCK_H_

#include <stddef.h>
#include <time.h>
#include <unistd.h>

#include "opensource/deps/base/macros.h"

namespace plusfish {

class Clock {
 public:
  Clock() {}
  virtual ~Clock() {}
  // Returns the amount of milliseconds passed since epoch.
  virtual size_t EpochTimeInMilliseconds() { return time(nullptr) * 1000; }
  // Sleep the amount of milliseconds.
  virtual void SleepMilliseconds(size_t ms) { usleep(ms * 1000); }

 private:
  DISALLOW_COPY_AND_ASSIGN(Clock);
};

}  // namespace plusfish

#endif  // THIRD_PARTY_PLUSFISH_UTIL_CLOCK_H_
