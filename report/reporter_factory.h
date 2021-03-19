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

#ifndef PLUSFISH_REPORT_REPORTER_FACTORY_H_
#define PLUSFISH_REPORT_REPORTER_FACTORY_H_

#include <string>

#include "opensource/deps/base/macros.h"
#include "proto/report.pb.h"
#include "report/reporter.h"

namespace plusfish {

// A factor class that can be used to get reporter instances.
class ReporterFactory {
 public:
  ReporterFactory() {}
  virtual ~ReporterFactory() {}

  // Return a reporter who's type matches the given name. Valid names are the
  // enum labels of ReportType (in report.proto).
  // Caller must take ownership of the returned instance.
  ReporterInterface* GetReporterByName(const std::string& name) const;

 private:
  // Get a reporter of the requested type.
  // Caller must take ownership of the returned instance.
  ReporterInterface* GetReporter(const ReportType& reporter_type) const;

  DISALLOW_COPY_AND_ASSIGN(ReporterFactory);
};

}  // namespace plusfish

#endif  // PLUSFISH_REPORT_REPORTER_FACTORY_H_
