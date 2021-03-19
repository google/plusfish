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

#ifndef PLUSFISH_AUDIT_SECURITY_NOT_FOUND_DETECTOR_H_
#define PLUSFISH_AUDIT_SECURITY_NOT_FOUND_DETECTOR_H_

#include <memory>
#include <vector>

#include "opensource/deps/base/macros.h"
#include "absl/container/node_hash_map.h"
#include "absl/container/node_hash_set.h"
#include "http_client.h"
#include "request.h"
#include "request_handler.h"
#include "util/html_fingerprint.h"

namespace plusfish {

// Collects fingerprints of responses the server gives for non-existing files.
// These are used during the scan to determine whether a page is present or not
// and useful for cases where servers do give 404 codes.
class NotFoundDetector : public RequestHandlerInterface {
 public:
  NotFoundDetector();
  ~NotFoundDetector() override {}

  // Set the HTTP client schedule callback.
  void SetHttpClientScheduleCallback(std::function<bool(Request*)> callback) {
    schedule_cb_ = callback;
  }

  // Set the datastore callback to store fingerprints.
  void SetDatastoreFingerprintCallback(
      std::function<void(const HtmlFingerprint*)> callback) {
    store_fingerprint_cb_ = callback;
  }

  const absl::node_hash_set<std::string>& probed_urls() { return probed_urls_; }

  // Add a URL to probe for 404 fingerprints.
  virtual bool AddUrl(const std::string& url);

  // Callback for completed requests.
  // The HTTP client will call this callback whenever a request has received a
  // response.
  // Does not take ownership.
  int RequestCallback(Request* req) override;

 private:
  // Holds the HTTP requests for this test;
  absl::node_hash_map<std::string, std::unique_ptr<Request>> requests_;
  // URLs that were probed.
  absl::node_hash_set<std::string> probed_urls_;
  // Number of completed requests.
  int num_completed_requests_;
  // The http client Schedule callback to schedule new requests.
  std::function<bool(Request*)> schedule_cb_;
  // The datastore callback for storing fingerprints.
  std::function<void(const HtmlFingerprint*)> store_fingerprint_cb_;

  DISALLOW_COPY_AND_ASSIGN(NotFoundDetector);
};

}  // namespace plusfish
#endif  // PLUSFISH_AUDIT_SECURITY_NOT_FOUND_DETECTOR_H_
