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

#ifndef PLUSFISH_AUDIT_SECURITY_HIDDEN_OBJECTS_FINDER_H_
#define PLUSFISH_AUDIT_SECURITY_HIDDEN_OBJECTS_FINDER_H_

#include <memory>
#include <queue>
#include <vector>

#include "opensource/deps/base/macros.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/node_hash_map.h"
#include "http_client.h"
#include "proto/issue_details.pb.h"
#include "request.h"
#include "request_handler.h"
#include "util/html_fingerprint.h"

namespace plusfish {

// Searches for hidden files and directories.
// Initialize with an HTTP client Schedule callback, a DataStore
// IsNotFoundHtmlFingerprint callback, a DataStore AddRequest callback and a
// DataStore AddIssue callback.
class HiddenObjectsFinder : public RequestHandlerInterface {
 public:
  // Default constructor for mocking.
  HiddenObjectsFinder() {}
  HiddenObjectsFinder(
      std::function<bool(Request*)> schedule_cb,
      std::function<bool(const HtmlFingerprint*)> is_html_fingerprint_cb,
      std::function<int64(std::unique_ptr<Request>)> add_request_cb,
      std::function<bool(const int64, const IssueDetails::IssueType,
                         const Severity)>
          add_issue_cb);

  ~HiddenObjectsFinder() override {}

  // Returns the number of completed requests.
  int number_completed_requests() const { return num_completed_requests_; }

  // Returns number of pending URLs.
  int pending_urls_count() const { return pending_urls_.size(); }

  // Returns the number of pending test requests.
  int test_urls_queue_count() const { return test_urls_queue_.size(); }

  // Returns the number of found objects.
  int num_objects_found() const { return num_objects_found_; }

  // Load words from a wordlist file. Each entry in the file
  // is expected to be in the format of <string> <int> where
  // the string represends the word and the int represents
  // either 1 or 0. If 1 is specified then the word will be
  // bruteforced with all possible extensions. With a 0 just
  // the word is used.
  int LoadWordlistFromFile(const std::string& wordlist_file);

  // Add and parse wordlist line into the wordlist map.
  bool AddWordlistLine(const std::string& wordlist_line);

  // Load the extensions from a file. Each extension has to
  // have a "." included when necessary. This makes it
  // easier to also test for suffixes such as ~.
  int LoadExtensionsFromFile(const std::string& extensions_file);

  // Add an extension to the extension list. Returns true is the extension was
  // added and false if it already existed.
  bool AddExtension(const std::string& extension);

  // Returns the URLs that have been tested.
  const absl::flat_hash_set<std::string>& probed_urls() { return probed_urls_; }

  // Add a URL to probe for 404 fingerprints.
  virtual void AddUrl(const std::string& url);

  // Schedule a maximum of "amount" new requests. Returns the number of test
  // URLs still pending to be scheduled.
  int64 ScheduleRequests(int32 amount);

  // Callback for completed requests.
  // The HTTP client will call this callback whenever a
  // request has received a response. Does not take
  // ownership.
  int RequestCallback(Request* req) override;

 private:
  // Load file content into the output hash set.
  int LoadListFromFile(const std::string& file,
                       absl::flat_hash_set<std::string>* output);
  // Uses the wordlist and extension to populate the test_urls_queue with URLs
  // to schedule requests for.  Returns the size of test_urls_queue.
  int64 GenerateTestUrls(const std::string& url);

  // The words to search for.
  absl::node_hash_map<std::string, bool> wordlist_;
  // The extensions to try together with the words.
  absl::flat_hash_set<std::string> extensions_;
  // Holds the HTTP requests for this test;
  absl::node_hash_map<std::string, std::unique_ptr<Request>> requests_;
  // URLs that were probed.
  absl::flat_hash_set<std::string> probed_urls_;
  // URLs for which no test URLs have been created and for which no requests
  // have been scheduled.
  std::queue<std::string> pending_urls_;
  // The current list of test URLs that have not been scheduled.
  std::queue<std::string> test_urls_queue_;
  // Number of completed requests.
  int num_completed_requests_;
  // Number of detected objects.
  int num_objects_found_;
  // The http client Schedule callback to schedule new
  // requests.
  std::function<bool(Request*)> schedule_cb_;
  // The datastore callback for storing fingerprints.
  std::function<bool(const HtmlFingerprint*)> is_html_fingerprint_cb_;
  // The callback to store a request in the datastore.
  std::function<int64(std::unique_ptr<Request>)> add_request_cb_;
  // The callback to report an issue to the datastore.
  std::function<bool(const int64, const IssueDetails::IssueType,
                     const Severity)>
      add_issue_cb_;

  DISALLOW_COPY_AND_ASSIGN(HiddenObjectsFinder);
};

}  // namespace plusfish
#endif  // PLUSFISH_AUDIT_SECURITY_HIDDEN_OBJECTS_FINDER_H_
