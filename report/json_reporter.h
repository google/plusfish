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

#ifndef PLUSFISH_REPORT_JSON_REPORTER_H_
#define PLUSFISH_REPORT_JSON_REPORTER_H_

#include <google/protobuf/util/json_util.h>

#include <memory>
#include <string>
#include <vector>

#include "opensource/deps/base/macros.h"
#include "proto/security_check.pb.h"
#include "report/reporter.h"
#include "util/file_writer.h"

namespace plusfish {

class Pivot;

// A JSON reporter.
// Reports the plusfish results in JSON. This class does not stop reporting when
// write errors occur (at the benefit of avoiding dataloss). This means that the
// created JSON files can be corrupt.
// This class is not thread safe.
class JSONReporter : public ReporterInterface {
 public:
  // Create an instance with the open FileWriter instance.
  // Takes ownership of the FileWriter.
  explicit JSONReporter(std::unique_ptr<FileWriter> file_writer);
  // Destroys the instance and closes the file handle.
  ~JSONReporter() override;

  // This causes a JSON array to be written to the file handle. The array
  // contains each request/response pair with details about issues that were
  // discovered.
  // The pivot response body is not stored in the report. We do store the
  // response body for issues though.
  // Does not take ownership.
  void ReportPivot(const Pivot* pivot, int depth) override;

  // Report the security check config.
  void ReportSecurityConfig(const SecurityCheckConfig& config) const override;

 private:
  // The filewriter instance to write the report with.
  std::unique_ptr<FileWriter> file_writer_;
  // A counter for the amount of (top level) items written in the report.
  int report_item_cnt_;
  // Json printing options
  struct google::protobuf::util::JsonPrintOptions json_options_;
  // Whether the pivot array prefix has been written.
  bool pivot_prefix_written_;
  DISALLOW_COPY_AND_ASSIGN(JSONReporter);
};

}  // namespace plusfish

#endif  // PLUSFISH_REPORT_JSON_REPORTER_H_
