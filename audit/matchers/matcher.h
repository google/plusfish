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

#ifndef PLUSFISH_AUDIT_MATCHERS_MATCHER_H_
#define PLUSFISH_AUDIT_MATCHERS_MATCHER_H_

#include "opensource/deps/base/macros.h"
#include "proto/matching.pb.h"
#include "request.h"

namespace plusfish {

// The interface which is implemented by all matchers. Matchers are
// reusable objects and not allowed to keep state between different matches.
class MatcherInterface {
 public:
  virtual ~MatcherInterface() {}

  // Whether the negative match results are expected.
  virtual bool negative() const = 0;

  // Allows the matcher to prepare for the matching. Returns true on success
  // and false on failure. On failure, the matcher should NOT be used.
  virtual bool Prepare() = 0;

  // Returns true when one of the Match values were found in the content.
  // During the matching, attributes from the Request can also be consulted
  // (e.g. timing values).
  // Does not take ownership.
  virtual bool MatchAny(const Request* request,
                        const std::string* content) const = 0;

 protected:
  MatcherInterface() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(MatcherInterface);
};

}  // namespace plusfish

#endif  // PLUSFISH_AUDIT_MATCHERS_MATCHER_H_
