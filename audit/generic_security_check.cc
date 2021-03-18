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

#include "audit/generic_security_check.h"

#include <glog/logging.h>
#include "audit/generator.h"
#include "audit/response_matcher.h"
#include "proto/security_check.pb.h"
#include "request.h"

namespace plusfish {

GenericSecurityCheck::GenericSecurityCheck(GeneratorInterface* generator,
                                           ResponseMatcherInterface* matcher,
                                           const SecurityTest& test)
    : security_test_(test), next_check_(nullptr) {
  generator_.reset(generator);
  matcher_.reset(matcher);
  DLOG(INFO) << "Created security check for: " << test.DebugString();
}

GenericSecurityCheck::~GenericSecurityCheck() {}

bool GenericSecurityCheck::CreateRequests(
    const Request& req, std::vector<std::unique_ptr<Request>>* requests) {
  if (generator_->Generate(&req, requests) == 0) {
    DLOG(INFO) << "No requests were generated for this test.";
    return false;
  }
  return true;
}

bool GenericSecurityCheck::Evaluate(
    const std::vector<std::unique_ptr<Request>>& requests) {
  return matcher_ != nullptr ? matcher_->Match(&requests) : false;
}

bool GenericSecurityCheck::EvaluateSingle(const Request* request) {
  return matcher_ != nullptr ? matcher_->MatchSingle(request) : false;
}

}  // namespace plusfish
