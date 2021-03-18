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

#include "hidden_objects_finder.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <glog/logging.h>
#include "absl/container/flat_hash_set.h"
#include "absl/container/node_hash_map.h"
#include "absl/strings/str_split.h"
#include "datastore.h"
#include "parsers/gumbo_filter.h"
#include "parsers/gumbo_fingerprint_filter.h"
#include "parsers/gumbo_parser.h"
#include "proto/http_response.pb.h"
#include "proto/issue_details.pb.h"
#include "request.h"
#include "util/url.h"

namespace plusfish {

HiddenObjectsFinder::HiddenObjectsFinder(
    std::function<bool(Request*)> schedule_cb,
    std::function<bool(const HtmlFingerprint*)> is_html_fingerprint_cb,
    std::function<int64(std::unique_ptr<Request>)> add_request_cb,
    std::function<bool(const int64, const IssueDetails::IssueType,
                       const Severity)>
        add_issue_cb)
    : num_completed_requests_(0),
      num_objects_found_(0),
      schedule_cb_(schedule_cb),
      is_html_fingerprint_cb_(is_html_fingerprint_cb),
      add_request_cb_(add_request_cb),
      add_issue_cb_(add_issue_cb) {}

int HiddenObjectsFinder::LoadListFromFile(
    const std::string& file, absl::flat_hash_set<std::string>* output) {
  std::ifstream is(file);
  if (!is.is_open()) {
    LOG(WARNING) << "Unable to open: " << file;
    return 0;
  }

  std::string content((std::istreambuf_iterator<char>(is)),
                      std::istreambuf_iterator<char>());

  std::vector<std::string> lines =
      absl::StrSplit(content, '\n', absl::SkipEmpty());
  output->insert(lines.begin(), lines.end());
  return output->size();
}

int HiddenObjectsFinder::LoadWordlistFromFile(
    const std::string& wordlist_file) {
  absl::flat_hash_set<std::string> entries;
  if (!LoadListFromFile(wordlist_file, &entries)) {
    return 0;
  }

  for (const auto& entry : entries) {
    if (!AddWordlistLine(entry)) {
      LOG(WARNING) << "Incorrect wordlist entry: " << entry;
      continue;
    }
  }
  return wordlist_.size();
}

bool HiddenObjectsFinder::AddWordlistLine(const std::string& wordlist_line) {
  std::vector<std::string> split_entry =
      absl::StrSplit(wordlist_line, absl::ByAnyChar(" \t"));
  if (split_entry.size() != 2) {
    return false;
  }

  if (split_entry[1].size() != 1 || !std::isdigit(split_entry[1][0])) {
    return false;
  }

  wordlist_[split_entry[0]] = std::stoi(split_entry[1]) ? true : false;
  return true;
}

bool HiddenObjectsFinder::AddExtension(const std::string& extension) {
  if (!extensions_.contains(extension)) {
    extensions_.insert(extension);
    return true;
  }
  return false;
}

int HiddenObjectsFinder::LoadExtensionsFromFile(
    const std::string& extensions_file) {
  return LoadListFromFile(extensions_file, &extensions_);
}

int64 HiddenObjectsFinder::ScheduleRequests(int32 amount) {
  if (test_urls_queue_.empty()) {
    if (pending_urls_.empty()) {
      return 0;
    }

    const std::string pending_url = pending_urls_.front();
    pending_urls_.pop();
    int64 generated = GenerateTestUrls(pending_url);
    LOG(INFO) << "Generated " << generated
              << " hidden objects targets for URL: " << pending_url;
  }

  for (int i = 0; i < amount && !test_urls_queue_.empty(); i++) {
    const std::string test_url = test_urls_queue_.front();
    test_urls_queue_.pop();

    std::unique_ptr<Request> req = absl::make_unique<Request>(test_url);
    req->set_request_handler(this);

    if (!schedule_cb_(req.get())) {
      LOG(WARNING) << "Unable to schedule probe for: " << test_url;
    } else {
      requests_[test_url] = std::move(req);
    }
  }

  return test_urls_queue_.size();
}

void HiddenObjectsFinder::AddUrl(const std::string& url) {
  pending_urls_.push(url);
}

int64 HiddenObjectsFinder::GenerateTestUrls(const std::string& url) {
  std::string base_url = util::StripUrlFileSuffix(url);
  if (probed_urls_.contains(url)) {
    return false;
  }

  probed_urls_.insert(url);
  for (const auto& word : wordlist_) {
    // First schedule just the word.
    std::string test_url = base_url + word.first;
    if (probed_urls_.contains(test_url)) {
      continue;
    }

    test_urls_queue_.push(test_url);

    // If no extension bruteforcing is desired; continue to the next word.
    if (!word.second) {
      continue;
    }

    for (const auto& ext : extensions_) {
      test_url = base_url + word.first + ext;
      if (probed_urls_.contains(test_url)) {
        continue;
      }

      test_urls_queue_.push(test_url);
    }
  }

  return test_urls_queue_.size();
}

int HiddenObjectsFinder::RequestCallback(Request* req) {
  if (!req->response()) {
    LOG(WARNING) << "Got no response for request: " << req->url();
    requests_.erase(req->url());
    return 0;
  }

  if (req->response()->proto().code() == HttpResponse::NOT_FOUND ||
      req->response()->proto().code() == HttpResponse::FORBIDDEN) {
    requests_.erase(req->url());
    return 1;
  }

  num_completed_requests_++;
  std::unique_ptr<GumboParser> parser = absl::make_unique<GumboParser>();
  std::unique_ptr<HtmlFingerprint> fp = absl::make_unique<HtmlFingerprint>();
  GumboFingerprintFilter fp_filter(std::move(fp));
  std::vector<GumboFilter*> filters = {&fp_filter};

  if (!parser->Parse(req->response()->body())) {
    LOG(WARNING) << "Unable to parse 404 request: " << req->url();
    requests_.erase(req->url());
    return 0;
  }

  parser->FilterDocument(filters);
  fp = fp_filter.GetFingerprint();

  if (!fp) {
    LOG(WARNING) << "Could not obtain fingerprint for: " << req->url();
    requests_.erase(req->url());
    return 0;
  }

  if (is_html_fingerprint_cb_(fp.get())) {
    requests_.erase(req->url());
    return 0;
  }

  LOG(INFO) << "Found file or directory: " << req->url();
  int64 request_id = add_request_cb_(std::move(requests_[req->url()]));
  requests_.erase(req->url());

  if (request_id != DataStore::kInvalidId) {
    add_issue_cb_(request_id, IssueDetails::OBJECT_FOUND, Severity::MODERATE);
    num_objects_found_++;
  }

  return 1;
}

}  // namespace plusfish
