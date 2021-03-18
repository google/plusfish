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

#include "parsers/html_scraper.h"

#include <glog/logging.h>
#include "absl/memory/memory.h"
#include "parsers/gumbo_filter.h"
#include "parsers/gumbo_fingerprint_filter.h"
#include "parsers/gumbo_form_filter.h"
#include "parsers/gumbo_parser.h"
#include "parsers/gumbo_url_filter.h"
#include "parsers/gumbo_xss_filter.h"
#include "request.h"

namespace plusfish {

HtmlScraper::HtmlScraper() {}

HtmlScraper::~HtmlScraper() {}

bool HtmlScraper::Parse(const Request* request,
                        const std::string& html_content) {
  gumbo_parser_ = absl::make_unique<GumboParser>();
  fingerprint_ = absl::make_unique<HtmlFingerprint>();

  GumboUrlFilter url_filter(&anchors_, &issues_);
  GumboXssFilter xss_filter(*request, &issues_);
  GumboFormFilter form_filter(request, &requests_, &issues_);
  GumboFingerprintFilter fp_filter(std::move(fingerprint_));

  std::vector<GumboFilter*> filters = {&url_filter, &xss_filter, &form_filter,
                                       &fp_filter};
  // If the content cannot be parsed, the parser doesn't initialize so we return
  // immediately.
  if (!gumbo_parser_->Parse(html_content)) {
    DLOG(WARNING) << "Could not parse content for URL: " << request->url();
    return false;
  }

  gumbo_parser_->FilterDocument(filters);
  gumbo_parser_->DestroyCurrentOutput();
  fingerprint_ = fp_filter.GetFingerprint();
  return true;
}

}  // namespace plusfish
