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

#include "audit/selective_auditor.h"

#include <functional>
#include <memory>

#include <glog/logging.h>
#include "audit/generic_generator.h"
#include "audit/generic_response_matcher.h"
#include "audit/generic_security_check.h"
#include "audit/matchers/matcher_factory.h"
#include "audit/security_check.h"
#include "audit/security_check_runner.h"
#include "http_client.h"
#include "proto/security_check.pb.h"

using std::placeholders::_1;

namespace plusfish {

SelectiveAuditor::SelectiveAuditor(const MatcherFactory* matcher_factory,
                                   HttpClientInterface* http_client)
    : matcher_factory_(matcher_factory),
      crawler_scrape_cb_(nullptr),
      http_client_(http_client) {}

SelectiveAuditor::~SelectiveAuditor() {}

// TODO: Move the check creation to a factory.
bool SelectiveAuditor::AddSecurityTest(const SecurityTest& sec_test) {
  std::unique_ptr<GenericResponseMatcher> matcher;
  if (sec_test.has_matching_rule()) {
    matcher.reset(
        new GenericResponseMatcher(sec_test.matching_rule(), matcher_factory_));
    if (!matcher->Init()) {
      LOG(WARNING) << "Unable to init response matcher from rule: "
                   << sec_test.matching_rule().DebugString();
      return false;
    }
  }
  LOG(INFO) << "Adding generic security check: " << sec_test.name();
  AddSecurityCheck(
      new GenericSecurityCheck(new GenericGenerator(sec_test.generator_rule()),
                               matcher.release(), sec_test));
  return true;
}

void SelectiveAuditor::AddSecurityCheck(SecurityCheckInterface* check) {
  check->SetRequestMetaCallback(set_req_meta_cb_);
  check->SetGetRequestMetaCallback(get_req_meta_cb_);
  if (!checks_.empty()) {
    checks_.back()->set_next(check);
  }
  checks_.emplace_back(check);
}

void SelectiveAuditor::SetCrawlerScrapeCallback(
    std::function<bool(const Request*)> callback) {
  crawler_scrape_cb_ = callback;
}

void SelectiveAuditor::SetRegisterIssueCallback(
    std::function<bool(const int64 request_id,
                       const IssueDetails::IssueType issue_type,
                       const Severity severity, const Request* test_request)>
        callback) {
  issue_callback_ = callback;
}

void SelectiveAuditor::SetRequestMetaCallback(
    std::function<bool(const int64 request_id, const MetaData_Type type,
                       const int64 value)>
        callback) {
  set_req_meta_cb_ = callback;
}

void SelectiveAuditor::SetGetRequestMetaCallback(
    std::function<bool(const int64 request_id, const MetaData_Type type,
                       int64* value)>
        callback) {
  get_req_meta_cb_ = callback;
}

bool SelectiveAuditor::ScheduleFirst(const Request* request) {
  if (checks_.empty()) {
    LOG(WARNING) << "SelectiveAuditor has not checks!";
    return false;
  }

  std::unique_ptr<SecurityCheckRunner> runner(
      new SecurityCheckRunner(checks_.begin()->get(), request));
  runner->OnCheckDone(std::bind(&SelectiveAuditor::FinishedCheckCb, this, _1));
  if (issue_callback_) {
    runner->SetRegisterIssueCallback(issue_callback_);
  }

  if (ScheduleNextCheck(runner.get())) {
    absl::MutexLock l(&runners_mutex_);
    runners_.push_back(std::move(runner));
    return true;
  }
  return false;
}

bool SelectiveAuditor::ScheduleNextCheck(SecurityCheckRunner* runner) {
  // Some checks will not run when there are simply no parameters to
  // inject a payload in. No problem: we move to the next check.
  while (!runner->Run(http_client_)) {
    if (!runner->SetNextCheck()) {
      DLOG(INFO) << "SecurityCheckRunne done for :"
                 << runner->tested_request()->url();
      return false;
    }
  }
  return true;
}

void SelectiveAuditor::FinishedCheckCb(SecurityCheckRunner* runner) {
  if (runner->requests_completed() && crawler_scrape_cb_) {
    DLOG(INFO) << "Scraping security check reqs #"
               << runner->requests_completed();
    for (const auto& req : runner->requests()) {
      crawler_scrape_cb_(req.get());
    }
  }

  if (runner->SetNextCheck() && ScheduleNextCheck(runner)) {
    DLOG(INFO) << "A new check was scheduled for: "
               << runner->tested_request()->url();
    return;
  }

  // Do housekeeping with a lock.
  absl::MutexLock l(&runners_mutex_);
  for (auto iter = runners_.begin(); iter != runners_.end(); ++iter) {
    if (iter->get() == runner) {
      runners_.erase(iter);
      return;
    }
  }
}

}  // namespace plusfish
