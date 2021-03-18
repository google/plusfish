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

#include "report/text_reporter.h"

#include <fstream>
#include <iostream>
#include <memory>

#include <glog/logging.h>
#include "pivot.h"
#include "proto/http_request.pb.h"
#include "proto/http_response.pb.h"
#include "proto/issue_details.pb.h"
#include "util/file_writer.h"

namespace plusfish {

TextReporter::TextReporter(std::unique_ptr<FileWriter> file_writer) {
  file_writer_ = std::move(file_writer);
}

TextReporter::~TextReporter() { file_writer_->Close(); }

void TextReporter::ReportPivot(const Pivot* pivot, int depth) {
  const std::vector<std::unique_ptr<Request>>& requests = pivot->requests();
  for (const auto& request : requests) {
    std::string req_string = request->url();

    // Build an std::string with extra details about the request.
    std::string details =
        HttpRequest::RequestMethod_Name(request->proto().method());
    if (request->response() != nullptr) {
      std::string response_code =
          HttpResponse::ResponseCode_Name(request->response()->proto().code());
      details.append(", " + response_code);
    } else {
      details.append(", not_fetched ");
    }
    req_string.append(" [" + details + "] ");

    if (!request->issues().empty()) {
      req_string.append("Issue: ");

      for (const auto& issue : request->issues()) {
        req_string.append(IssueDetails_IssueType_Name(issue.first) + " ");
      }
    }

    file_writer_->WriteString(req_string + "\n");
  }
}

}  // namespace plusfish
