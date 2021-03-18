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

#include "audit/generic_response_matcher.h"

#include <string>

#include <glog/logging.h>
#include "audit/matchers/condition_matcher.h"
#include "audit/matchers/matcher.h"
#include "audit/matchers/matcher_factory.h"
#include "proto/matching.pb.h"
#include "request.h"
#include "response.h"

namespace plusfish {

GenericResponseMatcher::GenericResponseMatcher(
    const MatchRule& match_rule, const MatcherFactory* matcher_factory)
    : match_rule_(match_rule), matcher_factory_(matcher_factory) {
  initialized_ = false;
  DLOG(INFO) << "Create matcher with rule: " << match_rule.DebugString();
}

GenericResponseMatcher::~GenericResponseMatcher() {}

bool GenericResponseMatcher::Init() {
  // Precompile the regular expressions to improve performance.
  // Also returns false when the regex is broken (so we can error out).
  for (const auto& condition : match_rule_.condition()) {
    std::unique_ptr<ConditionMatcher> condition_matcher(new ConditionMatcher());
    for (const auto& match : condition.match()) {
      std::unique_ptr<MatcherInterface> matcher(
          matcher_factory_->GetMatcher(match));
      if (!matcher || !matcher->Prepare()) {
        return false;
      }

      condition_matcher->AddMatcher(matcher.release());
    }
    matchers_.insert(std::make_pair(std::move(condition_matcher), condition));
  }
  initialized_ = true;
  return true;
}

bool GenericResponseMatcher::Match(
    const std::vector<std::unique_ptr<Request>>* requests) {
  DCHECK(initialized_);
  bool matched = false;
  for (const auto& request : *requests) {
    if (MatchSingle(request.get())) {
      matched = true;
    }
  }
  return matched;
}

bool GenericResponseMatcher::MatchSingle(const Request* req) {
  DCHECK(initialized_);
  const Response* resp = req->response();
  if (resp == nullptr) {
    DLOG(WARNING) << "No response for request: " << req->url();
    return false;
  }

  for (const auto& match_iter : matchers_) {
    const auto& condition_matcher = match_iter.first;
    const auto& condition = match_iter.second;

    const std::string* search_string = nullptr;
    switch (condition.target()) {
      case MatchRule_Target_RESPONSE_BODY:
        search_string = &resp->body();
        break;
      case MatchRule_Target_RESPONSE_HEADER_VALUE:
        // This may return nullptr when the header is not present.
        search_string = resp->GetHeader(condition.field());
        break;
      case MatchRule_Target_REQUEST_URL:
        search_string = &req->url();
        break;
      default:
        DLOG(WARNING) << "Matcher type not implemented" << condition.target();
        return false;
    }

    if (!condition_matcher->Match(req, search_string)) {
      return false;
    }
  }

  return true;
}

}  // namespace plusfish
