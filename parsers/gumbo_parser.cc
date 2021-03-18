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

#include "parsers/gumbo_parser.h"

#include <map>

#include <glog/logging.h>
#include <gumbo.h>

namespace plusfish {

GumboParser::GumboParser() : output_(nullptr) {}

GumboParser::~GumboParser() {
  if (output_ != nullptr) {
    DestroyCurrentOutput();
  }
}

GumboOutput* GumboParser::output() const { return output_; }

void GumboParser::DestroyCurrentOutput() {
  if (output_ == nullptr) {
    LOG(WARNING) << "Destroy called for unintialized Gumbo tree";
    return;
  }

  gumbo_destroy_output(&kGumboDefaultOptions, output_);
  output_ = nullptr;
}

void GumboParser::FilterDocument(std::vector<GumboFilter*> filters) {
  FilterDocumentNode(*output_->root, filters);
}

void GumboParser::FilterDocumentNode(const GumboNode& node,
                                     std::vector<GumboFilter*> filters) {
  const GumboVector* children = nullptr;

  for (auto& filter : filters) {
    switch (node.type) {
      case GUMBO_NODE_ELEMENT:
        filter->ParseElement(node.v.element);
        children = &node.v.element.children;
        break;
      case GUMBO_NODE_COMMENT:
        filter->ParseComment(node.v.text);
        break;
      case GUMBO_NODE_TEXT:
        filter->ParseText(node.v.text);
        break;
      default:
        DVLOG(1) << "Not filtering node: " << node.type;
        break;
    }
  }
  if (children == nullptr) {
    return;
  }
  for (unsigned int i = 0; i < children->length; ++i) {
    FilterDocumentNode(*static_cast<GumboNode*>(children->data[i]), filters);
  }
}

const GumboOutput* GumboParser::Parse(const std::string& buffer) {
  DLOG(INFO) << "Parsing: " << buffer;
  if (output_ != nullptr) {
    DestroyCurrentOutput();
  }
  output_ = gumbo_parse(buffer.c_str());
  return output_;
}

// static
GumboAttribute* GumboParser::GetAttribute(const GumboVector* attrs,
                                          const std::string& name) {
  return gumbo_get_attribute(attrs, name.c_str());
}

// static
void GumboParser::FillAttributeMap(
    const GumboElement& element,
    std::map<std::string, std::string>* output_map) {
  for (int i = 0; i < element.attributes.length; ++i) {
    GumboAttribute* attr =
        static_cast<GumboAttribute*>(element.attributes.data[i]);

    if (attr->value == nullptr) {
      output_map->emplace(attr->name, "");
    } else {
      output_map->emplace(attr->name, attr->value);
    }
  }
}

}  // namespace plusfish
