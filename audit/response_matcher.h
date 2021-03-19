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

#ifndef PLUSFISH_AUDIT_RESPONSE_MATCHER_H_
#define PLUSFISH_AUDIT_RESPONSE_MATCHER_H_

#include <vector>
#include <memory>

#include "opensource/deps/base/macros.h"
#include "opensource/deps/base/port.h"

namespace plusfish {

class Request;

class ResponseMatcherInterface {
 public:
  virtual ~ResponseMatcherInterface() {}
  // Initialize the class. Should be called before calling the match methods.
  virtual bool Init() MUST_USE_RESULT = 0;
  // Reviews the responses for the given requests. Returns true if the content
  // of one or more of these responses matches (with the implemented
  // expectations). Returns false is none of the responses matches.
  virtual bool Match(
      const std::vector<std::unique_ptr<Request>>* requests) = 0;
  // Same as above but only looks at a single request.
  // Does not take ownership.
  virtual bool MatchSingle(const Request* req) = 0;

 protected:
  ResponseMatcherInterface() { }
  bool initialized_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ResponseMatcherInterface);
};

}  // namespace plusfish

#endif  // PLUSFISH_AUDIT_RESPONSE_MATCHER_H_
