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

#include "audit/passive_auditor.h"

#include <functional>
#include <memory>

#include <glog/logging.h>
#include "audit/generic_response_matcher.h"
#include "proto/security_check.pb.h"

namespace plusfish {

PassiveAuditor::PassiveAuditor(const MatcherFactory* matcher_factory)
    : matcher_factory_(matcher_factory) {}

PassiveAuditor::~PassiveAuditor() {}

bool PassiveAuditor::AddSecurityTest(const SecurityTest& check) {
  std::unique_ptr<GenericResponseMatcher> matcher;
  matcher.reset(
      new GenericResponseMatcher(check.matching_rule(), matcher_factory_));
  if (!matcher->Init()) {
    LOG(WARNING) << "Unable to init response matcher from rule: "
                 << check.matching_rule().DebugString();
    return false;
  }

  LOG(INFO) << "Adding passive security check: " << check.name();
  response_matchers_.emplace(std::move(matcher), check);
  return true;
}

void PassiveAuditor::SetRegisterIssueCallback(
    std::function<bool(const int64 request_id,
                       const IssueDetails::IssueType issue_type,
                       const Severity severity, const Request* test_request)>
        callback) {
  issue_callback_ = callback;
}

bool PassiveAuditor::Check(Request* request) const {
  if (!request->response()) {
    return false;
  }

  for (const auto& matcher_iter : response_matchers_) {
    if (matcher_iter.first->MatchSingle(request)) {
      const auto& sectest = matcher_iter.second;
      // And report the issue. The issue callback might return false when one
      // of the thresholds is reached. We do not consider that an error here.
      if (issue_callback_(request->id(), sectest.issue_type(),
                          sectest.advisory().severity(), request)) {
        DLOG(INFO) << "Issue '" << sectest.name()
                   << "' detected on: " << request->url();
      }
    }
  }
  return true;
}

}  // namespace plusfish
