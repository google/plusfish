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

#include "response.h"

#include <glog/logging.h>
#include "google/protobuf/descriptor.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "proto/http_common.pb.h"
#include "proto/http_response.pb.h"
#include "util/http_util.h"

// The HTTP response header/body split string.
static const char* kHeaderBodySplitString = "\r\n\r\n";
static const char* kHeaderBodySplitStringNoCr = "\n\n";
// Invalid HTTP response code.
static const int kInvalidServerCode = -1;

namespace plusfish {

Response::Response() {}

Response::Response(const HttpResponse& response_proto) {
  http_response_ = response_proto;
}

Response::~Response() {}

const HttpResponse& Response::proto() const { return http_response_; }

const std::string& Response::body() const {
  return http_response_.response_body();
}

const std::string* Response::GetHeader(const std::string& header_name) const {
  for (const auto& header : http_response_.header()) {
    if (strncasecmp(header.name().c_str(), header_name.c_str(),
                    header_name.size()) == 0) {
      return &header.value();
    }
  }
  return nullptr;
}

const MimeInfo::MimeType Response::MimeType() const {
  return http_response_.mime_type();
}

bool Response::Equals(const Response& ref) const {
  // First low resource comparison.
  if (ref.proto().code() != http_response_.code() ||
      ref.MimeType() != http_response_.mime_type()) {
    return false;
  }

  // Second compare the full response body.
  if (ref.MimeType() == MimeInfo::ASC_HTML ||
      ref.MimeType() == MimeInfo::XML_XHTML) {
    if (html_fingerprint_ && ref.get_html_fingerprint()) {
      return html_fingerprint_->Equals(*ref.get_html_fingerprint());
    } else if ((html_fingerprint_ && !ref.get_html_fingerprint()) ||
               (!html_fingerprint_ && ref.get_html_fingerprint())) {
      // One of the responses has no fingerprint. This should not happen but in
      // this case we'll assume they are different.
      DLOG(ERROR)
          << "Unable to compare fingerprints; one fingerprint is missing.";
      return false;
    } else {
      // If both fingerprints are not set; this is an error condition
      // where we can't make the comparison.
      DLOG(ERROR) << "Unable to compare fingerprints; both are missing";
      return false;
    }
  }

  return http_response_.response_body() == ref.proto().response_body();
}

bool Response::Parse(const std::string& response) {
  size_t body_offset = 0;
  const char* separator = nullptr;
  // Split on whatever comes first: \r\n\r\n or \n\n.
  for (; body_offset < response.size(); ++body_offset) {
    if (response[body_offset] == '\r' &&
        response.substr(body_offset, 4) == kHeaderBodySplitString) {
      separator = kHeaderBodySplitString;
      break;
    } else if (response[body_offset] == '\n' &&
               response.substr(body_offset, 2) == kHeaderBodySplitStringNoCr) {
      separator = kHeaderBodySplitStringNoCr;
      break;
    }
  }
  if (separator == nullptr) {
    DLOG(INFO) << "Received response without header/body";
    return false;
  }

  std::string headers = response.substr(0, body_offset);
  http_response_.set_response_body(
      response.substr(body_offset + strlen(separator), std::string::npos));
  if (!ParseHeaders(headers)) {
    DLOG(WARNING) << "Unable to parse headers: " << headers;
    return false;
  }
  http_response_.set_mime_type(IdentifyMime());
  return true;
}

MimeInfo::MimeType Response::IdentifyMime() const {
  // TODO: Simulate browser mime sniffing.
  const std::string* content_type = GetHeader(HTTPHeaders::kContentType);
  return HTTPHeaders::GetMimeType(content_type);
}

bool Response::ParseHeaders(const std::string& headers) {
  std::vector<std::string> header_lines =
      absl::StrSplit(headers, absl::ByAnyChar("\r\n"), absl::SkipEmpty());
  std::vector<std::string> status_line = absl::StrSplit(header_lines[0], ' ');
  header_lines.erase(header_lines.begin());

  // Step 1: Parse the server line.
  if (status_line.size() < 3 || status_line[0].compare(0, 4, "HTTP") != 0) {
    LOG(WARNING) << "Response doesn't start with HTTP: " << status_line[0];
    return false;
  }

  int server_status_code = kInvalidServerCode;
  if (!absl::SimpleAtoi(status_line[1], &server_status_code) ||
      !HttpResponse::ResponseCode_IsValid(server_status_code) ||
      server_status_code == HttpResponse::UNKNOWN_CODE) {
    LOG(WARNING) << "Received unsupported HTTP server code: " << status_line[1];
    return false;
  }

  http_response_.set_code(HttpResponse::ResponseCode(server_status_code));

  // Step 2: Parse the headers.
  for (const auto& header_line : header_lines) {
    size_t colon_offset = header_line.find(':');
    if (colon_offset != std::string::npos) {
      HttpResponse::HeaderField* header = http_response_.add_header();
      header->set_name(header_line.substr(0, colon_offset));
      // Skip the : and space after the header name.
      if (header_line[++colon_offset] == ' ' &&
          colon_offset < header_line.size()) {
        ++colon_offset;
      }
      if (colon_offset < header_line.size()) {
        header->set_value(header_line.substr(colon_offset, std::string::npos));
      }
    }
  }
  return true;
}

}  // namespace plusfish
