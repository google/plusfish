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

#ifndef PLUSFISH_REPORT_TEXT_REPORTER_H_
#define PLUSFISH_REPORT_TEXT_REPORTER_H_

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "pivot.h"
#include "report/reporter.h"
#include "util/file_writer.h"

namespace plusfish {

class SecurityCheckConfig;

class TextReporter : public ReporterInterface {
 public:
  // Create a TextReporter by giving it a file handle that should be
  // writeable for the reporter instance.
  // Takes ownership of file_writer and closes the file when the reporter gets
  // destroyed.
  explicit TextReporter(std::unique_ptr<FileWriter> file_writer);
  ~TextReporter() override;

  // Report a single Pivot by printing it's details to file handle.
  void ReportPivot(const Pivot* pivot, int depth) override;

 private:
  std::string report_file_;
  std::unique_ptr<FileWriter> file_writer_;
  DISALLOW_COPY_AND_ASSIGN(TextReporter);
};

}  // namespace plusfish

#endif  // PLUSFISH_REPORT_TEXT_REPORTER_H_
