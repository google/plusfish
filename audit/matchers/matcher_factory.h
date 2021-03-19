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

#ifndef PLUSFISH_AUDIT_MATCHERS_MATCHER_FACTORY_H_
#define PLUSFISH_AUDIT_MATCHERS_MATCHER_FACTORY_H_

#include <memory>
#include <vector>

#include <functional>

#include "opensource/deps/base/macros.h"
#include "audit/matchers/matcher.h"
#include "proto/matching.pb.h"

namespace plusfish {

// A simple factory to get different Matcher flavors.
class MatcherFactory {
 public:
  MatcherFactory();
  virtual ~MatcherFactory() {}

  // Return a Matcher for the given MatchRule. On error, a nullptr is returned.
  // The caller has ownership of the returned matcher.
  virtual MatcherInterface* GetMatcher(const MatchRule_Match& match) const;

  // Set the datastore request metadata callback. This callback can be given to
  // matchers so that they have access to some of the request metadata.
  void SetRequestMetaCallback(
      std::function<bool(const int64 request_id, const MetaData_Type type,
                         int64* value)>
          callback) {
    get_req_meta_cb_ = callback;
  }

 private:
  // A datastore callback to get request specific metadata.
  std::function<bool(const int64 request_id, const MetaData_Type type,
                     int64* value)>
      get_req_meta_cb_;

  DISALLOW_COPY_AND_ASSIGN(MatcherFactory);
};

}  // namespace plusfish

#endif  // PLUSFISH_AUDIT_MATCHERS_MATCHER_FACTORY_H_
