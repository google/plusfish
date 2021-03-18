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

#include "parsers/gumbo_fingerprint_filter.h"

#include <ctype.h>

#include <memory>
#include <string>
#include <vector>

#include <glog/logging.h>
#include "absl/strings/str_split.h"
#include <gumbo.h>
#include "parsers/util/scrape_util.h"

namespace plusfish {

GumboFingerprintFilter::GumboFingerprintFilter(
    std::unique_ptr<HtmlFingerprint> fingerprint)
    : fingerprint_(std::move(fingerprint)) {}

GumboFingerprintFilter::~GumboFingerprintFilter() {}

void GumboFingerprintFilter::ParseElement(const GumboElement& node) {
  if (node.original_tag.data && node.original_tag.data[0] == '<') {
    std::string tag(node.original_tag.data + 1);
    int offset = tag.find_first_of("> ");
    if (offset != std::string::npos) {
      tag.resize(offset);
      fingerprint_->AddWord(tag);
    }
  }
  for (int i = 0; i < node.attributes.length; i++) {
    const auto& attr = *static_cast<GumboAttribute*>(node.attributes.data[i]);
    fingerprint_->AddWord(attr.name);
  }
}

bool GumboFingerprintFilter::IsAlphanumeric(const std::string& word) {
  return std::find_if(word.begin(), word.end(), [](char c) {
           return !(isalnum(c) || isspace(c));
         }) == word.end();
}

void GumboFingerprintFilter::ParseText(const GumboText& node) {
  std::string content(node.original_text.data, node.original_text.length);
  std::vector<std::string> chunks =
      absl::StrSplit(content, absl::ByAnyChar(" \t\n"));

  for (const auto& chunk : chunks) {
    if (IsAlphanumeric(chunk) && !chunk.empty()) {
      fingerprint_->AddWord(chunk);
    }
  }
}

}  //  namespace plusfish
