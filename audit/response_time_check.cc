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

#include "audit/response_time_check.h"

#include <glog/logging.h>
#include "proto/http_request.pb.h"
#include "request.h"

namespace plusfish {

// The name of the check.
static const char* const kCheckName = "ResponseTime";

ResponseTimeCheck::ResponseTimeCheck(int number_requests_to_create,
                                     int max_average_response_time_ms)
    : number_requests_to_create_(number_requests_to_create),
      max_average_response_time_ms_(max_average_response_time_ms),
      next_check_(nullptr),
      name_(kCheckName) {}

ResponseTimeCheck::~ResponseTimeCheck() {}

bool ResponseTimeCheck::CreateRequests(
    const Request& req, std::vector<std::unique_ptr<Request>>* requests) {
  for (int i = 0; i < number_requests_to_create_; ++i) {
    requests->emplace_back(new Request(req.proto()));
    requests->back()->set_parent_id(req.id());
  }
  return true;
}

bool ResponseTimeCheck::Evaluate(
    const std::vector<std::unique_ptr<Request>>& requests) {
  int64 duration_sum_usec = 0;
  for (const auto& request : requests) {
    duration_sum_usec += request->client_time_application_usec();
  }

  int64 average_duration_usec = duration_sum_usec / requests.size();
  DLOG(INFO) << "Setting average response time: " << average_duration_usec;
  if (set_req_meta_cb_(requests.front()->parent_id(),
                       MetaData_Type_AVERAGE_APPLICATION_TIME_USEC,
                       average_duration_usec)) {
  }
  return (average_duration_usec / 1000) > max_average_response_time_ms_;
}

bool ResponseTimeCheck::EvaluateSingle(const Request* request) {
  // Not supported.
  return false;
}

void ResponseTimeCheck::SetRequestMetaCallback(
    std::function<bool(const int64 request_id, const MetaData_Type type,
                       const int64 value)>
        callback) {
  set_req_meta_cb_ = callback;
}

void ResponseTimeCheck::SetGetRequestMetaCallback(
    std::function<bool(const int64 request_id, const MetaData_Type type,
                       int64* value)>
        callback) {
  get_req_meta_cb_ = callback;
}

}  // namespace plusfish
