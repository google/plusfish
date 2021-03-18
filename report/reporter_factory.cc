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

#include "report/reporter_factory.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include <glog/logging.h>
#include "absl/flags/flag.h"
#include "proto/report.pb.h"
#include "report/json_reporter.h"
#include "report/reporter.h"
#include "report/text_reporter.h"
#include "util/file_writer.h"

ABSL_FLAG(std::string, text_report_file, "/dev/stdout",
          "Specify where the results should be written to.");
ABSL_FLAG(std::string, json_report_file, "report.json",
          "The file to write the JSON report in.");

namespace plusfish {

ReporterInterface* ReporterFactory::GetReporter(
    const ReportType& reporter_type) const {
  switch (reporter_type) {
    case TEXT: {
      LOG(INFO) << "Creating text reporter for file: "
                << absl::GetFlag(FLAGS_text_report_file);

      std::unique_ptr<FileWriter> outfile(new FileWriter());
      if (!outfile->Open(absl::GetFlag(FLAGS_text_report_file))) {
        LOG(ERROR) << "Unable to open report file!";
        return nullptr;
      }

      return new TextReporter(std::move(outfile));
    }
    case JSON: {
      LOG(INFO) << "Creating JSON reporter for file: "
                << absl::GetFlag(FLAGS_json_report_file);

      std::unique_ptr<FileWriter> outfile(new FileWriter());
      if (!outfile->Open(absl::GetFlag(FLAGS_json_report_file))) {
        LOG(ERROR) << "Unable to open report file!";
        return nullptr;
      }

      return new JSONReporter(std::move(outfile));
    }
    default:
      LOG(WARNING) << "Did not receive a valid reporter type.";
  }
  return nullptr;
}

ReporterInterface* ReporterFactory::GetReporterByName(
    const std::string& name) const {
  ReportType report_type;
  if (!ReportType_Parse(name, &report_type)) {
    LOG(WARNING) << "Report type unknown: " << name;
    return nullptr;
  }
  return GetReporter(report_type);
}

}  // namespace plusfish
