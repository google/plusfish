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

#include "report/json_reporter.h"

#include <memory>

#include <glog/logging.h>
#include <google/protobuf/util/json_util.h>
#include "pivot.h"
#include "proto/http_request.pb.h"
#include "proto/http_response.pb.h"
#include "proto/report.pb.h"
#include "proto/security_check.pb.h"
#include "util/file_writer.h"

namespace plusfish {

// The config object prefix.
static const char* kConfigPrefix = "{\"config\": ";
// The JSON report prefix and suffix.
static const char* kPivotsPrefix = "\"pivots\": [";
static const char* kPivotsSuffix = "]}";
// The delimeter used to separate JSON array elements.
static const char* kJSONDelimeter = ",";

JSONReporter::JSONReporter(std::unique_ptr<FileWriter> file_writer)
    : report_item_cnt_(0), pivot_prefix_written_(false) {
  file_writer_ = std::move(file_writer);
  json_options_.add_whitespace = true;
  json_options_.always_print_primitive_fields = true;
}

JSONReporter::~JSONReporter() {
  DLOG(INFO) << "Number of requests written: " << report_item_cnt_;
  file_writer_->WriteString(kPivotsSuffix);
  file_writer_->Close();
}

void JSONReporter::ReportSecurityConfig(
    const SecurityCheckConfig& config) const {
  if (pivot_prefix_written_) {
    LOG(WARNING) << "Report must be written before the pivots.";
    return;
  }
  std::string config_string;
  if (!google::protobuf::util::MessageToJsonString(config, &config_string, json_options_)
           .ok()) {
    LOG(WARNING) << "Cannot convert config to JSON" << config.DebugString();
  }

  file_writer_->WriteString(kConfigPrefix + config_string + kJSONDelimeter);
}

void JSONReporter::ReportPivot(const Pivot* pivot, int depth) {
  if (!pivot_prefix_written_) {
    file_writer_->WriteString(kPivotsPrefix);
    pivot_prefix_written_ = true;
  }

  const std::vector<std::unique_ptr<Request>>& requests = pivot->requests();
  for (const auto& request : requests) {
    ReportItem details;
    *details.mutable_request() = request->proto();
    if (request->response() != nullptr) {
      *details.mutable_response() = request->response()->proto();
      // Keep the headers but remove the original response body to prevent the
      // report from filling the disc.
      details.mutable_response()->clear_response_body();
    }

    // If issues are present: add them.
    for (const auto& issue_iter : request->issues()) {
      for (const auto& issue : issue_iter.second) {
        details.add_issue()->MergeFrom(*issue);
      }
    }

    // Convert to JSON.
    std::string output_string;
    if (!google::protobuf::util::MessageToJsonString(details, &output_string,
                                           json_options_)
             .ok()) {
      LOG(WARNING) << "Unable to convert: " << details.DebugString();
      return;
    }

    if (report_item_cnt_ > 0) {
      file_writer_->WriteString(kJSONDelimeter);
    }

    file_writer_->WriteString(output_string);
    ++report_item_cnt_;
  }
}

}  // namespace plusfish
