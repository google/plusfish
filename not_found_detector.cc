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

#include "not_found_detector.h"

#include <memory>

#include <glog/logging.h>
#include "absl/container/node_hash_map.h"
#include "absl/random/distributions.h"
#include "absl/random/random.h"
#include "parsers/gumbo_filter.h"
#include "parsers/gumbo_fingerprint_filter.h"
#include "parsers/gumbo_parser.h"
#include "request.h"
#include "util/html_fingerprint.h"
#include "util/url.h"

namespace plusfish {

constexpr absl::string_view kExtensionsToProbe[] = {
    "html", "php", "jsp", "asp", "aspx", "rb", "cgi", "pl", "py"};

NotFoundDetector::NotFoundDetector() : num_completed_requests_(0) {
  DLOG(INFO) << "Created 404 detector";
}

bool NotFoundDetector::AddUrl(const std::string& url) {
  std::string base_url = util::StripUrlFileSuffix(url);

  if (probed_urls_.contains(base_url)) {
    return false;
  }

  absl::BitGen gen;
  int number = absl::Uniform(absl::IntervalClosedClosed, gen, 42, 41424344);
  for (const auto& ext : kExtensionsToProbe) {
    std::string target_url =
        base_url + std::to_string(number) + "." + std::string(ext);
    std::unique_ptr<Request> req = absl::make_unique<Request>(target_url);
    req->set_request_handler(this);
    if (!schedule_cb_(req.get())) {
      LOG(WARNING) << "Unable to schedule 404 probe for: " << target_url;
    } else {
      requests_[req->url()] = std::move(req);
      probed_urls_.insert(base_url);
    }
  }

  return true;
}

int NotFoundDetector::RequestCallback(Request* req) {
  if (!req->response()) {
    LOG(WARNING) << "Got no response for 404 request: " << req->url();
    requests_.erase(req->url());
    return 0;
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

  store_fingerprint_cb_(fp.get());
  requests_.erase(req->url());
  return 1;
}

}  // namespace plusfish
