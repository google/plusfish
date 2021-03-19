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

#ifndef OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_H_
#define OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_H_

#include <gumbo.h>

#include <map>
#include <memory>
#include <vector>

#include "opensource/deps/base/macros.h"
#include "parsers/gumbo_filter.h"

namespace plusfish {

// A basic wrapper for making the Gumbo HTML parser available (and mockable) in
// the c++ code. Example usage:
//   GumboParser gumbo_instance;
//   gumbo_instance.Parse("<html>");
class GumboParser {
 public:
  virtual ~GumboParser();
  GumboParser();
  // Getter for the GumboOutput attribute.
  GumboOutput* output() const;
  // Parse an HTML buffer. This returns the Gumbo output tree upon success. Upon
  // failure, NULL is returned.
  const GumboOutput* Parse(const std::string& buffer);
  // Iterates over the HTML document and run every node through the given
  // filters. Does not take ownership.
  void FilterDocument(std::vector<GumboFilter*> filters);
  // Runs the given GumboNode and (recursively) it's children through the
  // given filters.
  // Does not take ownership.
  void FilterDocumentNode(const GumboNode& node,
                          std::vector<GumboFilter*> filters);
  // Extract an attribute. If the argument is not found, NULL is returned.
  static GumboAttribute* GetAttribute(const GumboVector* attrs,
                                      const std::string& name);
  // Reads the attribute key/value pairs from the element into the target map.
  // Does not take ownership.
  static void FillAttributeMap(const GumboElement& element,
                               std::map<std::string, std::string>* output_map);
  // Destroy the HTML tree. This is also called by the default destructor and
  // therefore optional.
  void DestroyCurrentOutput();

 private:
  // A pointer to the Gumbo parsed tree. This is set and owned by the Gumbo
  // library.
  GumboOutput* output_;
  DISALLOW_COPY_AND_ASSIGN(GumboParser);
};

}  // namespace plusfish

#endif  // OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_PARSERS_GUMBO_H_
