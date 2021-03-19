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

#ifndef PLUSFISH_PIVOT_H_
#define PLUSFISH_PIVOT_H_

#include <stdio.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "opensource/deps/base/macros.h"
#include "report/reporter.h"
#include "request.h"

namespace plusfish {

// Pivot's don't have much logic within them and are mainly used by
// the crawler to a create a memory representation of a site. Every pivot
// represents an entry of the site's "sitemap" and holds several request
// samples for this entry.
class Pivot {
 public:
  Pivot();

  // Create a Pivot with the given name.
  explicit Pivot(const std::string& name);
  // Create a Pivot with the given name and also set the maximum amount of
  // Requests samples the pivot should store.
  Pivot(const std::string& name, int template_limit);
  // Same as above but with child limit set.
  Pivot(const std::string& name, int template_limit, int child_limit);
  virtual ~Pivot();

  // Get the pivot's name.
  const std::string& name() const { return pivot_name_; }

  const std::vector<std::unique_ptr<Request>>& requests() const {
    return req_samples_;
  }

  // Add a new request template to the pivot.
  // On success, this returns a pointer to the Request. Else nullptr is
  // returned.
  // Takes ownership of the given Request instance.
  Request* AddRequest(std::unique_ptr<Request> req);

  // Adds a child pivot and returns false when the children limit has been
  // reached.
  bool AddChildPivot(std::unique_ptr<Pivot> pivot);
  // The caller does not take ownership of the pointer.
  Pivot* GetChildPivot(const std::string& name);

  // Report the request details that are contained in this request. The depth
  // the position of the pivot in the crawler tree.
  // Does not take ownership.
  void Report(ReporterInterface* reporter, int depth);

 private:
  const int template_limit_;
  const int child_limit_;
  const std::string pivot_name_;

  std::map<std::string, std::unique_ptr<Pivot>> children_;
  std::vector<std::unique_ptr<Request>> req_samples_;
  DISALLOW_COPY_AND_ASSIGN(Pivot);
};

}  // namespace plusfish

#endif  // PLUSFISH_PIVOT_H_
