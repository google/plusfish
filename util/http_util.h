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

#ifndef PLUSFISH_UTIL_HTTP_UTIL_H_
#define PLUSFISH_UTIL_HTTP_UTIL_H_

#include <string>

#include "base/macros.h"
#include "proto/http_common.pb.h"

namespace plusfish {

// This is a utility class for common HTTP request/response attributes.
class HTTPHeaders {
 public:
  HTTPHeaders();
  virtual ~HTTPHeaders();

  // A content type std::string is given (e.g. "text/html"). If a matching
  // MimeInfo::MimeType exist, then this is returned. Else a
  // MimeInfo::UNKNOWN_MIME is returned.
  static MimeInfo::MimeType GetMimeType(const std::string* content_type);

  // Common HTTP headers names.
  static const char* const kContentType;
  static const char* const kUserAgent;
  static const char* const kLocation;
  static const char* const kReferer;

 private:
  DISALLOW_COPY_AND_ASSIGN(HTTPHeaders);
};

}  // namespace plusfish

#endif  // PLUSFISH_UTIL_HTTP_UTIL_H_
