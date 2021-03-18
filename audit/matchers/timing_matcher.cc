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

#include "audit/matchers/timing_matcher.h"

#include <string.h>
#include <string>

#include <glog/logging.h>
#include "audit/matchers/matcher.h"
#include "proto/http_request.pb.h"
#include "proto/matching.pb.h"
#include "request.h"

namespace plusfish {

TimingMatcher::TimingMatcher(
    const MatchRule_Match& match,
    std::function<bool(const int64 request_id, const MetaData_Type type,
                       int64* value)>
        get_req_meta_cb)
    : match_(match), get_req_meta_cb_(get_req_meta_cb) {}

bool TimingMatcher::Prepare() {
  return match_.has_timing() &&
         match_.timing().min_duration_ms() < match_.timing().max_duration_ms();
}

bool TimingMatcher::MatchAny(const Request* request,
                             const std::string* content) const {
  int64 average_application_time_usec = 0;
  int64 min_duration_ms = match_.timing().min_duration_ms();
  int64 max_duration_ms = match_.timing().max_duration_ms();
  if (get_req_meta_cb_(request->parent_id(),
                       MetaData_Type_AVERAGE_APPLICATION_TIME_USEC,
                       &average_application_time_usec)) {
    min_duration_ms += average_application_time_usec / 1000;
    max_duration_ms += average_application_time_usec / 1000;
    DLOG(INFO) << "Using modified expectation: min=" << min_duration_ms
               << ", max=" << max_duration_ms
               << ", avg_usec=" << average_application_time_usec;
  }

  return (request->client_time_application_usec() / 1000 > min_duration_ms &&
          request->client_time_application_usec() / 1000 < max_duration_ms);
}

}  // namespace plusfish
