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

#include "audit/security_check_runner.h"

#include <glog/logging.h>
#include "audit/security_check.h"
#include "http_client.h"
#include "proto/issue_details.pb.h"
#include "request.h"

namespace plusfish {

SecurityCheckRunner::SecurityCheckRunner(SecurityCheckInterface* check,
                                         const Request* request)
    : finished_(false),
      requests_completed_(0),
      tested_request_(request),
      security_check_(check),
      check_done_callback_(nullptr),
      issue_callback_(nullptr) {
  DLOG(INFO) << "Created security runner for: " << request->url();
}

bool SecurityCheckRunner::Run(HttpClientInterface* http_client) {
  if (!security_check_->CreateRequests(*tested_request_, &requests_)) {
    DLOG(INFO) << "Unable to create requests for: " << security_check_->name();
    return false;
  }

  if (security_check_->CanEvaluateInSerial() &&
      security_check_->EvaluateSingle(tested_request_)) {
    LOG(WARNING) << "Original request already tests positive: skipping.";
    return false;
  }

  for (const auto& req : requests_) {
    if (!http_client->Schedule(req.get(), this)) {
      LOG(WARNING) << "Unable to schedule request: "
                   << req->proto().DebugString();
      // For serial evaluation, a failed request is acceptable.
      if (!security_check_->CanEvaluateInSerial()) {
        LOG(WARNING) << "Skipping security test: " << security_check_->name()
                     << " for: " << req->url();
        return false;
      }
    }
  }
  return true;
}

bool SecurityCheckRunner::SetNextCheck() {
  if (security_check_->next() == nullptr) {
    return false;
  }
  security_check_ = security_check_->next();

  // Clear state.
  requests_.clear();
  finished_ = false;
  requests_completed_ = 0;
  return true;
}

int SecurityCheckRunner::RequestCallback(Request* req) {
  if (++requests_completed_ == requests_.size()) {
    finished_ = true;
  }
  if (security_check_->CanEvaluateInSerial()) {
    if (security_check_->EvaluateSingle(req) && issue_callback_) {
      issue_callback_(req->parent_id(), security_check_->issue_type(),
                      security_check_->severity(), req);
    }
  } else if (finished_ && security_check_->Evaluate(requests_) &&
             issue_callback_) {
    issue_callback_(req->parent_id(), security_check_->issue_type(),
                    security_check_->severity(), req);
  }

  if (finished_ && check_done_callback_ != nullptr) {
    DLOG(INFO) << "Calling cleanup routing";
    // This destroys the current object: do not use 'this' after the next line!
    check_done_callback_(this);
  }
  // TODO: remove the required int return value.
  return 0;
}

void SecurityCheckRunner::OnCheckDone(
    std::function<void(SecurityCheckRunner*)> callback) {
  check_done_callback_ = callback;
}

void SecurityCheckRunner::SetRegisterIssueCallback(
    std::function<bool(const int64 request_id,
                       const IssueDetails::IssueType issue_type,
                       const Severity severity, const Request* test_request)>
        callback) {
  issue_callback_ = callback;
}

}  // namespace plusfish
