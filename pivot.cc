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

#include "pivot.h"

#include <glog/logging.h>
#include "absl/flags/flag.h"
#include "report/reporter.h"

ABSL_FLAG(int32_t, pivot_request_limit, 100,
          "Maximum requests samples to store in a single pivot.");
ABSL_FLAG(int32_t, pivot_child_limit, 100,
          "Maximum children a single pivot will store.");

namespace plusfish {

Pivot::Pivot()
    : template_limit_(absl::GetFlag(FLAGS_pivot_request_limit)),
      child_limit_(absl::GetFlag(FLAGS_pivot_child_limit)) {}

Pivot::Pivot(const std::string& name)
    : template_limit_(absl::GetFlag(FLAGS_pivot_request_limit)),
      child_limit_(absl::GetFlag(FLAGS_pivot_child_limit)),
      pivot_name_(name) {
  DLOG(INFO) << "Pivot: " << pivot_name_ << " created";
}

Pivot::Pivot(const std::string& name, int template_limit)
    : template_limit_(template_limit),
      child_limit_(absl::GetFlag(FLAGS_pivot_child_limit)),
      pivot_name_(name) {
  DLOG(INFO) << "Pivot: " << pivot_name_ << " created";
}

Pivot::Pivot(const std::string& name, int template_limit, int child_limit)
    : template_limit_(template_limit),
      child_limit_(child_limit),
      pivot_name_(name) {
  DLOG(INFO) << "Pivot: " << pivot_name_ << " created";
}

Pivot::~Pivot() { DLOG(INFO) << "Pivot: " << pivot_name_ << " destructor"; }

Request* Pivot::AddRequest(std::unique_ptr<Request> req) {
  if (req_samples_.size() >= template_limit_) {
    LOG_EVERY_N(WARNING, 10)
        << "Pivot: " << pivot_name_ << " - reached the request limit";
    return nullptr;
  }

  // Check the request is not already present
  for (auto req_ptr = req_samples_.begin(); req_ptr != req_samples_.end();
       req_ptr++) {
    if (req_ptr->get()->Equals(*(req))) return nullptr;
  }

  req_samples_.emplace_back(req.release());
  DLOG(INFO) << "Pivot: " << pivot_name_ << " - added new request";

  // Return a pointer to the request.
  return req_samples_.back().get();
}

bool Pivot::AddChildPivot(std::unique_ptr<Pivot> pivot) {
  if (children_.size() >= child_limit_) {
    LOG_EVERY_N(WARNING, 10)
        << "Pivot: " << pivot_name_ << " - reached the child limit";
    return false;
  }

  // Already exists? return false.
  if (GetChildPivot(pivot->name())) {
    DLOG_EVERY_N(INFO, 10) << "Pivot already present. Name: " << pivot->name();
    return false;
  }

  DLOG(INFO) << "Child pivot: " << pivot->name() << " - is added";
  children_.emplace(pivot->name(), std::move(pivot));
  return true;
}

Pivot* Pivot::GetChildPivot(const std::string& name) {
  if (children_.find(name) == children_.end()) {
    return nullptr;
  }

  return children_[name].get();
}

void Pivot::Report(ReporterInterface* reporter, int depth) {
  ++depth;
  for (const auto& child : children_) {
    reporter->ReportPivot(child.second.get(), depth);
    child.second->Report(reporter, depth);
  }
}

}  // namespace plusfish
