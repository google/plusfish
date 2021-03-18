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

#ifndef PLUSFISH_REPORT_REPORTER_H_
#define PLUSFISH_REPORT_REPORTER_H_

#include "base/macros.h"

namespace plusfish {

class Pivot;
class SecurityCheckConfig;

// Generic report interface. Classes implementing this interface can be used to
// report the scan results.
//
// Example usage:
//   ReporterImpl reporter;
//   for (const Pivot& : pivot_iterator) {
//     reporter.ReportPivot(pivot);
//   }
class ReporterInterface {
 public:
  virtual ~ReporterInterface() {}

  // Report a single Pivot. The depth parameter is used to indicate the
  // position of the pivot in the site tree.
  virtual void ReportPivot(const Pivot* pivot, int depth) = 0;

  // Report the security check config. Not all reporters are expected to
  // implement this so the default implementation has no logic.
  virtual void ReportSecurityConfig(const SecurityCheckConfig& config) const { }

 protected:
  ReporterInterface() { }

 private:
  DISALLOW_COPY_AND_ASSIGN(ReporterInterface);
};

}  // namespace plusfish

#endif  // PLUSFISH_REPORT_REPORTER_H_
