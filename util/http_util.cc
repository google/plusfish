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

#include "util/http_util.h"

#include "absl/strings/str_split.h"
#include "proto/http_common.pb.h"

namespace plusfish {

const char* const HTTPHeaders::kContentType = "Content-type";
const char* const HTTPHeaders::kUserAgent = "User-Agent";
const char* const HTTPHeaders::kLocation = "Location";
const char* const HTTPHeaders::kReferer = "Referer";

// The struct used to map mime names against a MimeInfo enum value.
struct MimeStruct {
  const char* name;
  const MimeInfo::MimeType type;
};

// HTTP request/response content mime types.
static const MimeStruct kContentTypes[] = {
    {"text/plain", MimeInfo::ASC_GENERIC},
    {"text/html", MimeInfo::ASC_HTML},
    {"text/xml", MimeInfo::XML_GENERIC},
    {"text/rtf", MimeInfo::ASC_RTF},
    {"text/vnd.wap.wml", MimeInfo::XML_WML},
    {"text/x-cross-domain-policy", MimeInfo::XML_CROSSDOMAIN},
    {"text/css", MimeInfo::ASC_CSS},
    {"text/json", MimeInfo::ASC_JSON},
    {"text/javascript", MimeInfo::ASC_JAVASCRIPT},
    {"image/svg+xml", MimeInfo::XML_SVG},
    {"image/jpeg", MimeInfo::IMG_JPEG},
    {"image/gif", MimeInfo::IMG_GIF},
    {"image/png", MimeInfo::IMG_PNG},
    {"image/x-ms-bmp", MimeInfo::IMG_BMP},
    {"image/tiff", MimeInfo::IMG_TIFF},
    {"image/svg+xml", MimeInfo::XML_SVG},
    {"video/avi", MimeInfo::AV_AVI},
    {"video/mpeg", MimeInfo::AV_MPEG},
    {"video/quicktime", MimeInfo::AV_QT},
    {"video/flv", MimeInfo::AV_FLV},
    {"video/vnd.rn-realvideo", MimeInfo::AV_RV},
    {"video/x-ms-wmv", MimeInfo::AV_WMEDIA},
    {"audio/x-wav", MimeInfo::AV_WAV},
    {"audio/mpeg", MimeInfo::AV_MP3},
    {"audio/vnd.rn-realaudio", MimeInfo::AV_RA},
    {"application/binary", MimeInfo::APP_GENERIC},
    {"application/unknown", MimeInfo::APP_UNKNOWN},
    {"application/octet-stream", MimeInfo::APP_OCTET},
    {"application/javascript", MimeInfo::APP_JAVASCRIPT},
    {"application/x-javascript", MimeInfo::APP_JAVASCRIPT},
    {"application/json", MimeInfo::APP_JSON},
    {"application/postscript", MimeInfo::APP_POSTSCRIPT},
    {"application/rss+xml", MimeInfo::XML_RSS},
    {"application/atom+xml", MimeInfo::XML_ATOM},
    {"application/xhtml+xml", MimeInfo::XML_XHTML},
    {"application/x-shockwave-flash", MimeInfo::EXT_FLASH},
    {"application/pdf", MimeInfo::EXT_PDF},
    {"application/java-archive", MimeInfo::EXT_JAR},
    {"application/java-vm", MimeInfo::EXT_CLASS},
    {"application/msword", MimeInfo::EXT_WORD},
    {"application/vnd.ms-excel", MimeInfo::EXT_EXCEL},
    {"application/vnd.ms-powerpoint", MimeInfo::EXT_PPNT},
    {"application/zip", MimeInfo::BIN_ZIP},
    {"application/x-gzip", MimeInfo::BIN_GZIP},
    {"application/x-navi-animation", MimeInfo::IMG_ANI},
    {"application/vnd.ms-cab-compressed", MimeInfo::BIN_CAB},
    {"application/ogg", MimeInfo::AV_OGG},
    {"application/opensearchdescription+xml", MimeInfo::XML_OPENSEARCH}};

HTTPHeaders::HTTPHeaders() {}

HTTPHeaders::~HTTPHeaders() {}

MimeInfo::MimeType HTTPHeaders::GetMimeType(const std::string* content_type) {
  if (content_type == nullptr) {
    return MimeInfo::UNKNOWN_MIME;
  }

  std::vector<std::string> mime_parts = absl::StrSplit(*content_type, ';');
  for (const auto& mime_info : kContentTypes) {
    if (strcasecmp(mime_info.name, mime_parts[0].c_str()) == 0) {
      return mime_info.type;
    }
  }
  return MimeInfo::UNKNOWN_MIME;
}

}  // namespace plusfish
