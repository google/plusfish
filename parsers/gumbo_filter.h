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

#ifndef OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_FILTER_H_
#define OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_FILTER_H_

#include <gumbo.h>

#include "opensource/deps/base/macros.h"

namespace plusfish {

// Parses and extracts information from Gumbo document nodes.
// GumboFilter implementations can be given to the Gumbo class to receive
// (and parse) all nodes in a document.
class GumboFilter {
 public:
  virtual ~GumboFilter() {}
  // Receives every GumboElement node from the document being parsed.
  // Does not take ownership.
  virtual void ParseElement(const GumboElement& node) = 0;
  // Receives every comment from the document being parsed as a GumboText node.
  // Does not take ownership.
  virtual void ParseComment(const GumboText& node) = 0;
  // Receives every GumboText node from the document being parsed.
  // Does not take ownership.
  virtual void ParseText(const GumboText& node) = 0;

 protected:
  GumboFilter() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(GumboFilter);
};

}  // namespace plusfish

#endif  // OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_FILTER_H_
