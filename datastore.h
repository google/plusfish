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

#ifndef PLUSFISH_DATASTORE_H_
#define PLUSFISH_DATASTORE_H_

#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/integral_types.h"
#include "base/macros.h"
#include "absl/base/thread_annotations.h"
#include "absl/container/node_hash_set.h"
#include "absl/synchronization/mutex.h"
#include "proto/http_request.pb.h"
#include "proto/issue_details.pb.h"
#include "request.h"
#include "util/html_fingerprint.h"
#include "re2/re2.h"

namespace plusfish {

class Pivot;
class ReporterInterface;

class DataStore {
 public:
  // TODO: Allow a seed to be given for the initial request ID.
  DataStore();
  virtual ~DataStore();

  // Returns the site pivots. Each contains a pivot tree of the tested site.
  const std::map<std::string, std::unique_ptr<Pivot>>& site_pivots() const {
    return site_pivots_;
  }

  // Return the issue severity counters
  const std::map<Severity, uint32_t> issue_count_per_severity() const {
    return issue_count_per_severity_;
  }

  // Returns the size of the audit queue.
  virtual size_t audit_queue_size() {
    absl::ReaderMutexLock r(&audit_queue_mutex_);
    return audit_queue_.size();
  }

  // Returns the size of the crawl queue.
  virtual size_t crawl_queue_size() {
    absl::ReaderMutexLock r(&crawl_queue_mutex_);
    return crawl_queue_.size();
  }

  // Returns the size of the probe queue.
  virtual size_t probe_queue_size() {
    absl::ReaderMutexLock r(&probe_queue_mutex_);
    return probe_queue_.size();
  }

  // Add a host. The datastore will only store Pivots & Requests for known
  // hosts and so calling this method is needed to include a host in scans.
  virtual void AddHost(const std::string& domain_or_ip);

  // Blacklist a URL if it matches this regex. Returns false if the regex could
  // not be compiled. Else true is returned.
  bool AddBlacklistRegex(const std::string& url_regex);

  // Whitelist all URLs that match this regex. If one or more whitelist URLs
  // are set; all other (non-matching) URLs are implicitly blacklisted.
  // This can still be used in combination with BlacklistUrl. For example,
  // you can whitelist /application and blacklist /application/foo to have
  // everything under /application scanned with the exception of
  // /application/foo.
  // Returns false if the regex cannot be compiled. Else true is returned.
  bool AddWhitelistRegex(const std::string& url_regex);

  // Add a new request to the datastore. Newly added Requests are given an ID
  // and this is also returned on success. When the request is not added -1 is
  // returned (e.g. when a limit is exceeded).
  const int64 AddRequest(std::unique_ptr<Request> req);

  // Lookup a request by it's ID. Returns nullptr of the request cannot be
  // found.
  virtual Request* GetRequestById(int64 id);

  // Iterate over pivots/requests and feed them to the reporters.
  virtual void Report(
      const std::vector<std::unique_ptr<ReporterInterface>>& reporters);

  // Set the callback for adding URLs to the 404 fingerprinter.
  virtual void SetFingerprinterCallback(
      std::function<bool(const std::string&)> callback) {
    fingerprinter_cb_ = callback;
  }

  // Set the callback for the hidden objects detector.
  virtual void SetHiddenObjectsFinderCallback(
      std::function<bool(const std::string&)> callback) {
    objects_finder_cb_ = callback;
  }

  // Register a security issue that was discovered with test_request and include
  // a specific Request to the report.
  // Returns true if the issue was added. If the issue cannot be added (e.g. a
  // limit for issues exceeded), false is returned.
  // Does not take ownership.
  bool AddIssue(const int64 request_id,
                const IssueDetails::IssueType issue_type,
                const Severity severity, const Request* test_request);

  // Register a security issue that was discovered with test_request.
  // Returns true if the issue was added. If the issue cannot be added (e.g. a
  // limit for issues exceeded), false is returned.
  bool AddIssueById(const int64 request_id,
                    const IssueDetails::IssueType issue_type,
                    const Severity severity);

  // Same as above. Takes ownership of the issue.
  bool AddIssueDetails(const int64 request_id,
                       std::unique_ptr<IssueDetails> issue);

  // Add metadata to a stored request.
  // Returns false when a matching entry already exists.
  virtual bool AddRequestMetadata(const int64 request_id,
                                  const MetaData_Type type, const int64 value);

  // Read the metadata entry into 'value'.
  // Returns false if the entry does not exist.
  virtual bool GetRequestMetadata(const int64 request_id,
                                  const MetaData_Type type, int64* value);

  // Add an HTML response fingerprint to the Request,
  virtual bool AddResponseFingerprintToRequest(
      const int64 request_id, std::unique_ptr<HtmlFingerprint> fingerprint);

  // Store an HTML 404 fingerprint. Some servers reply a non-404 to a file it
  // could not found. We probe for such responses and record the fingerprints to
  // use them during the scan.
  virtual void AddFileNotFoundHtmlFingerprint(
      const HtmlFingerprint* fingerprint);

  // Check if a fingerprint is for a 404.
  virtual bool IsFileNotFoundHtmlFingerprint(
      const HtmlFingerprint* fingerprint);

  // Adds Request 'req' to the audit queue.
  // Does not take ownership.
  virtual void AddRequestToAuditQueue(const Request* req);

  // Adds Request 'req' to the crawl queue.
  // Does not take ownership.
  virtual void AddRequestToCrawlQueue(const Request* req);

  // Adds Request 'req' to the probe queue.
  // Does not take ownership.
  virtual void AddRequestToProbeQueue(const Request* req);

  // Returns the oldest Request from the audit queue or nullptr when
  // the queue is empty.
  virtual const Request* GetRequestFromAuditQueue();

  // Returns the oldest Request from the crawl queue or nullptr when
  // the queue is empty.
  virtual const Request* GetRequestFromCrawlQueue();

  // Returns the oldest Request from the probe queue or nullptr when
  // the queue is empty.
  virtual const Request* GetRequestFromProbeQueue();

  // An invalid ID value which, when returned, indicates a failure status.
  static constexpr int64 kInvalidId = -1;

 private:
  // The internal method for adding a request to the pivot tree. This takes
  // ownership of the request and synchronizes access to the tree.
  // Returns the assigned request ID.
  const int64 AddRequestToPivot(std::unique_ptr<Request> req);
  // Add the given request to the "request by id" map.
  const int64 AddRequestToIdMap(Request* req);
  // Updates the global (issues per) security check counters. Returns false is
  // the global limit for the given issue has been reached. Else true is
  // returned.
  bool CheckAndUpdateIssueCounters(const IssueDetails::IssueType issue_type);

  // Hosts whitelisted for scanning.
  absl::node_hash_set<std::string> allowed_hosts_;
  // URLs matching these regexes will be excluded.
  absl::node_hash_set<std::unique_ptr<RE2>> url_blacklist_regexes_;
  // URLs matching these regexes will be included.
  absl::node_hash_set<std::unique_ptr<RE2>> url_whitelist_regexes_;
  // Site Pivots, stored per hostname.
  std::map<std::string, std::unique_ptr<Pivot>> site_pivots_
      ABSL_GUARDED_BY(site_pivots_mutex_);
  // A map storing pointers to all requests by ID.
  std::map<int64, Request*> requests_by_id_
      ABSL_GUARDED_BY(request_by_id_mutex_);
  // A map storing issues per Request ID. For each request ID, a map
  // containing IssueDetails are stored per issue type. E.g: {1 , {"XSS", {
  // <details1, details2, .. > } }.
  std::map<int64, std::map<IssueDetails::IssueType,
                           absl::node_hash_set<std::unique_ptr<IssueDetails>>>>
      issue_per_id_ ABSL_GUARDED_BY(issue_per_id_mutex_);
  // Map storing counters per severity
  std::map<Severity, uint32_t> issue_count_per_severity_;
  // Request metadata, such as measurements.
  std::map<int64, std::map<MetaData::Type, int64>> request_meta_
      ABSL_GUARDED_BY(request_meta_mutex_);
  absl::Mutex request_meta_mutex_;
  // The request ID counter.
  int64 request_id_ ABSL_GUARDED_BY(request_by_id_mutex_);
  // Stores the amount of detected issues per security check.
  std::map<IssueDetails::IssueType, int64> check_issue_counters_
      ABSL_GUARDED_BY(check_issue_counters_mutex_);
  // A FIFO queue which contains Requests that are ready for security testing.
  std::queue<const Request*> audit_queue_ ABSL_GUARDED_BY(audit_queue_mutex_);
  // A FIFO queue which contains Requests that are ready for crawling.
  std::queue<const Request*> crawl_queue_ ABSL_GUARDED_BY(crawl_queue_mutex_);
  // A FIFO queue which contains Requests that are ready for 404 and hidden
  // objects probing.
  std::queue<const Request*> probe_queue_ ABSL_GUARDED_BY(probe_queue_mutex_);
  // Synchronize access to the site pivots.
  absl::Mutex site_pivots_mutex_;
  // Synchronize access to the request_by_id map.
  absl::Mutex request_by_id_mutex_;
  // Synchronize access to the issue per request ID map.
  absl::Mutex issue_per_id_mutex_;
  // Synchronize access to the global (issues per) security check counters.
  absl::Mutex check_issue_counters_mutex_;
  // Synchronize access to the auditor queue.
  absl::Mutex audit_queue_mutex_;
  // Synchronize access to the probe queue.
  absl::Mutex probe_queue_mutex_;
  // Synchronize access to the crawl queue.
  absl::Mutex crawl_queue_mutex_;
  // File not found fingerprints
  absl::node_hash_set<std::string> not_found_fingerprints_;
  // The callback for the 404 fingerprinter.
  std::function<bool(const std::string&)> fingerprinter_cb_;
  // The callback for the hidden objects finder.
  std::function<bool(const std::string&)> objects_finder_cb_;
  DISALLOW_COPY_AND_ASSIGN(DataStore);
};

}  // namespace plusfish

#endif  // PLUSFISH_DATASTORE_H_
