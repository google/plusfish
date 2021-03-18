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

#ifndef OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_FP_FILTER_H_
#define OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_FP_FILTER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include <gumbo.h>
#include "parsers/gumbo_filter.h"
#include "util/html_fingerprint.h"

namespace plusfish {

class Request;

// This HTML parser filters words out of tags, attributes and text in order to
// feed them to the HtmlFingerprint instance.
//
// Example usage:
//    GumboFingerprintFilter gumbo_fingerprint_filter(std::move(fingerprint));
//    gumbo_fingerprint_filter.ParseElement(GumboElement);
//    // Get the final fingerprint
//    std::unique_ptr<HtmlFingerprint> fingerprint =
//         gumbo_fingerprint_filter.GetFingerprint();
class GumboFingerprintFilter : public GumboFilter {
 public:
  explicit GumboFingerprintFilter(std::unique_ptr<HtmlFingerprint> fingerprint);
  ~GumboFingerprintFilter() override;

  // Parses the element and adds it's name and attribute names to the
  // fingerprint.
  void ParseElement(const GumboElement& node) override;
  // Comments are not used for fingerprinting.
  void ParseComment(const GumboText& node) override {}
  // Parses the text and adds each word to the fingerprint.
  void ParseText(const GumboText& node) override;

  // Get fingerprint.
  std::unique_ptr<HtmlFingerprint> GetFingerprint() {
    return std::move(fingerprint_);
  }

 private:
  // Returns true if the word is alphanumeric and without spaces.
  bool IsAlphanumeric(const std::string& word);
  // The fingerprint that will be created.
  std::unique_ptr<HtmlFingerprint> fingerprint_;
  // All detected issues are added to this set.
  DISALLOW_COPY_AND_ASSIGN(GumboFingerprintFilter);
};

}  //  namespace plusfish

#endif  // OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_FP_FILTER_H_
