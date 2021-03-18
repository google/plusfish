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

#include "parsers/gumbo_url_filter.h"

#include <memory>
#include <string>
#include <vector>

#include <glog/logging.h>
#include "base/macros.h"
#include "absl/container/node_hash_set.h"
#include "absl/strings/match.h"
#include <gumbo.h>
#include "parsers/gumbo_parser.h"
#include "parsers/util/escaping.h"
#include "parsers/util/html_name.h"
#include "parsers/util/scrape_util.h"

// The javascript scheme used to identify JavaScript URLs.
static const char* kJavascriptScheme = "javascript:";

// The vbscript scheme used to identify vbscript URLs.
static const char* kVbscriptScheme = "vbscript:";

// The prefix we use to scrape the URL from a meta refresh element.
static const char* kMetaRefreshUrlPrefix = "url=";

// The prefix used to guess whether an attribute can be an event handler.
static const char* kEventHandlerPrefix = "on";

// A list of prefixes that attribute values are tested against. If a value
// matches, then we will try to parse it as an URL.
static const char* kPotentialUrlPrefixes[] = {"http://", "https://", "//"};

namespace plusfish {

GumboUrlFilter::GumboUrlFilter(
    std::vector<std::string>* anchors,
    absl::node_hash_set<std::unique_ptr<IssueDetails>>* issues)
    : anchors_(anchors), issues_(issues) {}

GumboUrlFilter::~GumboUrlFilter() {}

void GumboUrlFilter::AddAnchorsSet(
    const absl::node_hash_set<std::string>& anchors) {
  for (const auto& anchor : anchors) {
    AddAnchor(anchor);
  }
}

void GumboUrlFilter::AddAnchorsSetHtmlUnescaped(
    const absl::node_hash_set<std::string>& anchors) {
  for (const auto& anchor : anchors) {
    AddAnchor(util::UnescapeHtml(anchor));
  }
}

bool GumboUrlFilter::ParseMeta(const GumboElement& element) {
  const GumboAttribute* content_attr =
      GumboParser::GetAttribute(&element.attributes, HTMLName::kContent);

  // Stop unless there is an http-equiv and content attribute.
  if (content_attr == nullptr ||
      GumboParser::GetAttribute(&element.attributes, HTMLName::kHttpEquiv) ==
          nullptr) {
    return false;
  }

  const char* url = strcasestr(content_attr->value, kMetaRefreshUrlPrefix);
  if (url == nullptr) {
    return false;
  }

  // Although less common, the URL part can be quoted.
  std::string final_url = url + strlen(kMetaRefreshUrlPrefix);
  char quote = final_url.front();
  if (quote == '"' || quote == '\'') {
    if (final_url.back() == quote) {
      final_url = final_url.substr(1, final_url.length() - 2);
    } else {
      final_url = final_url.substr(1, final_url.length() - 1);
    }
  }

  AddAnchor(final_url);
  return true;
}

void GumboUrlFilter::AddAnchor(const std::string& new_anchor) {
  if (anchors_ == nullptr) {
    LOG(WARNING) << "Unable to store anchor: " << new_anchor;
    return;
  }

  if (std::find(anchors_->begin(), anchors_->end(), new_anchor) ==
      anchors_->end()) {
    // Anchors starting with "javascript:" will be scraped as JavaScript.
    if (absl::StartsWithIgnoreCase(new_anchor.c_str(), kJavascriptScheme)) {
      DLOG(INFO) << "Parsing JavaScript anchor: " << new_anchor;
      absl::node_hash_set<std::string> inline_js_anchors;
      util::ScrapeJs(
          new_anchor.substr(strlen(kJavascriptScheme), std::string::npos),
          &inline_js_anchors, issues_);
      AddAnchorsSet(inline_js_anchors);
      return;
    }

    // Anchors starting with "vbscript:" will also be scraped as JavaScript.
    // Although this might be less successful, it is good enough to detect XSS
    // injections.
    if (absl::StartsWithIgnoreCase(new_anchor.c_str(), kVbscriptScheme)) {
      DLOG(INFO) << "Parsing vbscript anchor: " << new_anchor;
      absl::node_hash_set<std::string> inline_vb_anchors;
      util::ScrapeJs(
          new_anchor.substr(strlen(kVbscriptScheme), std::string::npos),
          &inline_vb_anchors, issues_);
      AddAnchorsSet(inline_vb_anchors);
      return;
    }

    DLOG(INFO) << "Storing anchor: " << new_anchor;
    anchors_->push_back(new_anchor);
  }
}

void GumboUrlFilter::ParseElement(const GumboElement& node) {
  // Attributes names known to contain useful links. This is not intended
  // to be a complete list of attributes that can potentially contain URLs.
  static const std::vector<const char*> kAnchorAttributes = {
      plusfish::HTMLName::kHref, plusfish::HTMLName::kSrc,
      plusfish::HTMLName::kAction, plusfish::HTMLName::kUrl};

  last_element_ = &node;
  if (node.tag == GUMBO_TAG_META) {
    if (ParseMeta(node)) {
      return;
    }
  }

  for (int i = 0; i < node.attributes.length; ++i) {
    bool found = false;
    const GumboAttribute* attr =
        static_cast<GumboAttribute*>(node.attributes.data[i]);

    // Parse event handlers (onclick, onload, ..).
    if (absl::StartsWithIgnoreCase(attr->name, kEventHandlerPrefix)) {
      absl::node_hash_set<std::string> js_urls;
      util::ScrapeJs(attr->value, &js_urls, issues_);
      AddAnchorsSetHtmlUnescaped(js_urls);
    }
    // Grep values from the known attributes.
    for (const char* attr_name : kAnchorAttributes) {
      if (absl::StartsWithIgnoreCase(attr_name, attr->name)) {
        AddAnchor(util::UnescapeHtml(attr->value));
        found = true;
        break;
      }
    }

    // If we already extracted the value, continue to the next.
    if (found) {
      continue;
    }

    // Grep any attribute value that looks like a URL.
    for (int prefix_index = 0; prefix_index < arraysize(kPotentialUrlPrefixes);
         ++prefix_index) {
      if (strncmp(kPotentialUrlPrefixes[prefix_index], attr->value,
                  strlen(kPotentialUrlPrefixes[prefix_index])) == 0) {
        AddAnchor(util::UnescapeHtml(attr->value));
        break;
      }
    }
  }
}

void GumboUrlFilter::ParseComment(const GumboText& node) {
  absl::node_hash_set<std::string> anchors = util::ScrapeUrl(node.text);
  AddAnchorsSet(anchors);
}

void GumboUrlFilter::ParseText(const GumboText& node) {
  absl::node_hash_set<std::string> anchors;
  // Script tags get an additional (special) treatment.
  if (last_element_->tag && last_element_->tag == GUMBO_TAG_SCRIPT) {
    absl::node_hash_set<std::string> js_urls;
    util::ScrapeJs(node.text, &js_urls, issues_);
    anchors.insert(js_urls.begin(), js_urls.end());
  } else {
    absl::node_hash_set<std::string> new_anchors = util::ScrapeUrl(node.text);
    anchors.insert(new_anchors.begin(), new_anchors.end());
  }
  // TODO: decode based on content source type.
  AddAnchorsSet(anchors);
}
}  //  namespace plusfish
