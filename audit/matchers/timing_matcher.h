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

#ifndef PLUSFISH_AUDIT_MATCHERS_TIMING_MATCHER_H_
#define PLUSFISH_AUDIT_MATCHERS_TIMING_MATCHER_H_

#include <functional>

#include "base/macros.h"
#include "audit/matchers/matcher.h"
#include "proto/matching.pb.h"
#include "request.h"

namespace plusfish {

// Can be used to check request timing expectations.
class TimingMatcher : public MatcherInterface {
 public:
  // Create an instance with match rule and a datastore callback. The calback is
  // used by the matcher to retrieve the average response time.
  TimingMatcher(const MatchRule_Match& match,
                std::function<bool(const int64 request_id,
                                   const MetaData_Type type, int64* value)>
                    get_req_meta_cb);
  ~TimingMatcher() override {}

  // If negative matching is expected.
  bool negative() const override { return match_.negative_match(); }

  // Prepare the matcher. Returns false when the min and max request duration
  // values are incorrect.
  bool Prepare() override;

  // Returns true if any of the expected request duration is measured.
  // Does not take ownership.
  bool MatchAny(const Request* request,
                const std::string* content) const override;

 private:
  const MatchRule_Match match_;
  // The datastore callback to get request metadata.
  std::function<bool(const int64 request_id, const MetaData_Type type,
                     int64* value)>
      get_req_meta_cb_;
  DISALLOW_COPY_AND_ASSIGN(TimingMatcher);
};

}  // namespace plusfish

#endif  // PLUSFISH_AUDIT_MATCHERS_TIMING_MATCHER_H_
