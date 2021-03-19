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

#ifndef PLUSFISH_RESPONSE_H_
#define PLUSFISH_RESPONSE_H_

#include <stdio.h>
#include <stdlib.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "opensource/deps/base/macros.h"
#include "proto/http_common.pb.h"
#include "proto/http_response.pb.h"
#include "util/html_fingerprint.h"

namespace plusfish {

class Response {
 public:
  Response();
  // Allow initialisation using an existing response protobuf.
  explicit Response(const HttpResponse& response_proto);
  virtual ~Response();

  // Set the response body.
  void set_body(const std::string& body) {
    http_response_.set_response_body(body);
  }

  const HtmlFingerprint* get_html_fingerprint() const {
    return html_fingerprint_.get();
  }

  void set_html_fingerprint(std::unique_ptr<HtmlFingerprint> fingerprint) {
    html_fingerprint_ = std::move(fingerprint);
  }

  // Returns the response proto message.
  const HttpResponse& proto() const;
  // Returns the response body as a string.
  const std::string& body() const;
  // Get the value for the specified header. Returns a pointer to the header
  // value when a value is present. Else nullptr is returned.
  const std::string* GetHeader(const std::string& header_name) const;

  // Returns the mime type of the response.
  const MimeInfo::MimeType MimeType() const;

  // Parse the response into the encapsulated HttpResponse instance. The
  // response is expected to contain both headers and (optional) body.
  // Returns True if parsing was successful. Else False.
  bool Parse(const std::string& response);

  // Compares against the given Response instance. Returns True when
  // the referenced response is the same. Else False.
  bool Equals(const Response& ref) const;

 private:
  // Identify the response mime type by parsing Content-Type and return the
  // matching MimeType value. Returns MimeType::UNKNOWN_MIME when the mime is
  // unknown.
  MimeInfo::MimeType IdentifyMime() const;

  // Parse the given headers into the HttpResponse instance.
  // The given std::string should only contain the headers returned by the
  // server (not the response body). Returns True on success, else False.
  bool ParseHeaders(const std::string& headers);

  // The HttpResponse protobuf.
  HttpResponse http_response_;

  // The HTML fingerprint. This is only set when the mime of the response is for
  // HTML.
  std::unique_ptr<HtmlFingerprint> html_fingerprint_;
  DISALLOW_COPY_AND_ASSIGN(Response);
};

}  // namespace plusfish

#endif  // PLUSFISH_RESPONSE_H_
