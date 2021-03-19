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

#include "datastore.h"

#include <glog/logging.h>
#include <google/protobuf/util/message_differencer.h>

#include <functional>
#include <memory>

#include "absl/flags/flag.h"
#include "opensource/deps/base/integral_types.h"
#include "pivot.h"
#include "proto/http_common.pb.h"
#include "proto/http_request.pb.h"
#include "proto/issue_details.pb.h"
#include "proto/severity.pb.h"
#include "re2/re2.h"
#include "request.h"
#include "response.h"
#include "util/html_fingerprint.h"
#include "util/http_util.h"

ABSL_FLAG(int32_t, max_issues_per_check_per_url, 3,
          "Limit the total amount of issues a single security check can "
          "report for a single URL.");
ABSL_FLAG(int32_t, max_issues_per_url, 25,
          "Limit the total amount of issues per URL.");
ABSL_FLAG(int32_t, max_issues_per_check, 100,
          "Limit the total amount of issues a single security check can "
          "report in a single scan.");

namespace plusfish {

DataStore::DataStore() : request_id_(0) {
  // Initialize the severity counters. These counters are updated whenever an
  // Issue is added to the datastore.
  issue_count_per_severity_[Severity::CRITICAL] = 0;
  issue_count_per_severity_[Severity::HIGH] = 0;
  issue_count_per_severity_[Severity::MODERATE] = 0;
  issue_count_per_severity_[Severity::LOW] = 0;
  issue_count_per_severity_[Severity::MINIMAL] = 0;
  issue_count_per_severity_[Severity::UNKNOWN] = 0;
}

DataStore::~DataStore() { DLOG(INFO) << "DataStore stopped."; }

void DataStore::AddHost(const std::string& domain_or_ip) {
  LOG(INFO) << "Whitelisting host: " << domain_or_ip;
  allowed_hosts_.emplace(domain_or_ip);
}

bool DataStore::AddBlacklistRegex(const std::string& url_regex) {
  LOG(INFO) << "Adding blacklist regex: " << url_regex;
  std::unique_ptr<RE2> compiled_regex(new RE2(url_regex));
  if (!compiled_regex->ok()) {
    LOG(WARNING) << "Could not compile blacklist regex: " << url_regex
                 << " Error: " << compiled_regex->error();
    return false;
  }
  url_blacklist_regexes_.emplace(std::move(compiled_regex));
  return true;
}

bool DataStore::AddWhitelistRegex(const std::string& url_regex) {
  LOG(INFO) << "Adding whitelist regex: " << url_regex;
  std::unique_ptr<RE2> compiled_regex(new RE2(url_regex));
  if (!compiled_regex->ok()) {
    LOG(WARNING) << "Could not compile whitelist regex: " << url_regex
                 << " Error: " << compiled_regex->error();
    return false;
  }
  url_whitelist_regexes_.emplace(std::move(compiled_regex));
  return true;
}

// Add a request to the pivot tree (and update the tree).
const int64 DataStore::AddRequest(std::unique_ptr<Request> req) {
  // Make sure the host is whitelisted.
  if (!std::count(allowed_hosts_.begin(), allowed_hosts_.end(), req->host())) {
    DLOG(INFO) << "Skipping host: " << req->host();
    return DataStore::kInvalidId;
  }

  // Reject non-whitelisted URLs.
  for (const auto& regex : url_whitelist_regexes_) {
    if (!RE2::PartialMatch(req->url(), *regex)) {
      DLOG(INFO) << "Skipping non-whitelisted URL: " << req->url();
      return DataStore::kInvalidId;
    }
  }

  // Reject blacklisted URLs.
  for (const auto& regex : url_blacklist_regexes_) {
    if (RE2::PartialMatch(req->url(), *regex)) {
      DLOG(INFO) << "Skipping blacklisted URL: " << req->url();
      return DataStore::kInvalidId;
    }
  }

  // Check if the request has a valid URL.
  if (!req->url_is_valid()) {
    DLOG(INFO) << "Skipping incomplete request.";
    return DataStore::kInvalidId;
  }

  return AddRequestToPivot(std::move(req));
}

const int64 DataStore::AddRequestToPivot(std::unique_ptr<Request> req) {
  Pivot *pivot, *next_pivot;
  absl::MutexLock l(&site_pivots_mutex_);

  // First find the host pivot
  std::string name = req->host();
  if (site_pivots_.find(name) == site_pivots_.end()) {
    DLOG(INFO) << "Adding host pivot: " << name;
    site_pivots_.emplace(name, std::unique_ptr<Pivot>(new Pivot(name)));
  }

  pivot = site_pivots_[name].get();
  for (const auto& path : req->proto().path()) {
    next_pivot = pivot->GetChildPivot(path.value());
    if (next_pivot) {
      pivot = next_pivot;
      continue;
    }

    // Add the missing pivots.
    next_pivot = new Pivot(path.value());
    if (!pivot->AddChildPivot(std::unique_ptr<Pivot>(next_pivot))) {
      LOG(WARNING) << "Failed to add child pivot.";
      return DataStore::kInvalidId;
    }
    pivot = next_pivot;
  }

  // The "pivot" pointer points to the last segment. We now try to add the
  // request to the templates that this pivot stores.
  Request* request = pivot->AddRequest(std::move(req));
  if (request == nullptr) {
    DLOG(INFO) << "Pivot rejected";
    return DataStore::kInvalidId;
  }

  return AddRequestToIdMap(request);
}

const int64 DataStore::AddRequestToIdMap(Request* req) {
  absl::MutexLock r(&request_by_id_mutex_);

  ++request_id_;
  req->set_id(request_id_);
  requests_by_id_[request_id_] = req;

  AddRequestToProbeQueue(req);
  return req->id();
}

Request* DataStore::GetRequestById(int64 id) {
  absl::ReaderMutexLock r(&request_by_id_mutex_);
  if (requests_by_id_.count(id)) {
    return requests_by_id_[id];
  }
  return nullptr;
}

bool DataStore::AddIssue(const int64 request_id,
                         const IssueDetails::IssueType issue_type,
                         const Severity severity, const Request* test_request) {
  std::unique_ptr<IssueDetails> issue_details(new IssueDetails());
  *issue_details->mutable_request() = test_request->proto();
  issue_details->set_type(issue_type);
  issue_details->set_severity(severity);

  if (test_request->response() != nullptr) {
    *issue_details->mutable_response() = test_request->response()->proto();
  }
  return AddIssueDetails(request_id, std::move(issue_details));
}

bool DataStore::AddIssueById(const int64 request_id,
                             const IssueDetails::IssueType issue_type,
                             const Severity severity) {
  std::unique_ptr<IssueDetails> issue_details(new IssueDetails());

  Request* test_request = GetRequestById(request_id);
  *issue_details->mutable_request() = test_request->proto();
  issue_details->set_type(issue_type);
  issue_details->set_severity(severity);

  if (test_request->response() != nullptr) {
    *issue_details->mutable_response() = test_request->response()->proto();
  }
  return AddIssueDetails(request_id, std::move(issue_details));
}

bool DataStore::CheckAndUpdateIssueCounters(
    const IssueDetails::IssueType issue_type) {
  absl::MutexLock c(&check_issue_counters_mutex_);
  if (!check_issue_counters_.count(issue_type)) {
    check_issue_counters_[issue_type] = 1;
    return true;
  }
  if (check_issue_counters_[issue_type] ==
      absl::GetFlag(FLAGS_max_issues_per_check)) {
    return false;
  }
  check_issue_counters_[issue_type] += 1;
  return true;
}

bool DataStore::AddIssueDetails(const int64 request_id,
                                std::unique_ptr<IssueDetails> issue) {
  Request* req = GetRequestById(request_id);
  if (req == nullptr) {
    // This can happen when the issue ID comes from an external source. E.g.
    // when derived from an injected XSS tag.
    DLOG(ERROR) << "Cannot register issue for unknown request #: " << request_id
                << " " << issue->DebugString();
    return false;
  }

  // Check if one of the limits is reached.
  if (req->issues().size() == absl::GetFlag(FLAGS_max_issues_per_url)) {
    LOG(WARNING) << "Maximum issue limit reached for request ID: " << request_id
                 << ". Discarding: " << issue->DebugString();
    return false;
  }
  absl::MutexLock l(&issue_per_id_mutex_);
  if (issue_per_id_.count(request_id)) {
    auto& issue_map = issue_per_id_[request_id];
    if (issue_map.find(issue->type()) != issue_map.end()) {
      for (const auto& issue_entry : issue_map[issue->type()]) {
        // When there is an exact match, return.
        if (google::protobuf::util::MessageDifferencer::Equals(*issue_entry,
                                                               *issue)) {
          return false;
        }
      }
    }

    if (issue_map[issue->type()].size() ==
        absl::GetFlag(FLAGS_max_issues_per_check_per_url)) {
      LOG(WARNING) << "Maximum issue limit reached for check: "
                   << issue->issue_name()
                   << ". Discarding: " << issue->DebugString();
      return false;
    }
  }

  DLOG(INFO) << "Detected issue on request #: " << request_id
             << " Details: " << issue->DebugString();
  // Update the global security check counters.
  if (!CheckAndUpdateIssueCounters(issue->type())) {
    LOG(WARNING) << "Maximum global issue limit reached for check: "
                 << issue->issue_name()
                 << ". Discarding: " << issue->DebugString();
    return false;
  }
  issue_count_per_severity_[issue->severity()]++;
  req->AddIssue(issue.get());
  issue_per_id_[request_id][issue->type()].emplace(std::move(issue));
  return true;
}

bool DataStore::AddRequestMetadata(const int64 request_id,
                                   const MetaData_Type type,
                                   const int64 value) {
  absl::MutexLock l(&request_meta_mutex_);
  if (request_meta_.count(request_id) &&
      request_meta_[request_id].count(type)) {
    return false;
  }
  DLOG(INFO) << "Setting metadata: " << value << " Request: " << request_id;
  request_meta_[request_id][type] = value;
  return true;
}

bool DataStore::GetRequestMetadata(const int64 request_id,
                                   const MetaData_Type type, int64* value) {
  absl::ReaderMutexLock r(&request_meta_mutex_);
  if (!request_meta_.count(request_id) ||
      !request_meta_[request_id].count(type)) {
    return false;
  }
  *value = request_meta_[request_id][type];
  return true;
}

bool DataStore::AddResponseFingerprintToRequest(
    const int64 request_id, std::unique_ptr<HtmlFingerprint> fingerprint) {
  Request* req = GetRequestById(request_id);

  if (req == nullptr) {
    DLOG(ERROR) << "Cannot register fingerprint for unknown request #: "
                << request_id;
    return false;
  }

  req->set_response_html_fingerprint(std::move(fingerprint));
  return true;
}

void DataStore::AddFileNotFoundHtmlFingerprint(
    const HtmlFingerprint* fingerprint) {
  std::string fingerprint_str;
  fingerprint->ToString(&fingerprint_str);
  not_found_fingerprints_.insert(fingerprint_str);
}

bool DataStore::IsFileNotFoundHtmlFingerprint(
    const HtmlFingerprint* fingerprint) {
  if (!fingerprint) {
    return false;
  }
  std::string fingerprint_str;
  fingerprint->ToString(&fingerprint_str);
  return not_found_fingerprints_.contains(fingerprint_str);
}

void DataStore::AddRequestToAuditQueue(const Request* req) {
  DCHECK(req);
  absl::MutexLock l(&audit_queue_mutex_);
  audit_queue_.push(req);
}

void DataStore::AddRequestToCrawlQueue(const Request* req) {
  DCHECK(req);
  absl::MutexLock l(&crawl_queue_mutex_);
  crawl_queue_.push(req);
}

void DataStore::AddRequestToProbeQueue(const Request* req) {
  DCHECK(req);
  absl::MutexLock l(&probe_queue_mutex_);
  probe_queue_.push(req);
}

const Request* DataStore::GetRequestFromAuditQueue() {
  absl::MutexLock l(&audit_queue_mutex_);
  if (audit_queue_.empty()) {
    return nullptr;
  }

  const Request* ret = audit_queue_.front();
  audit_queue_.pop();
  return ret;
}

const Request* DataStore::GetRequestFromCrawlQueue() {
  absl::MutexLock l(&crawl_queue_mutex_);
  if (crawl_queue_.empty()) {
    return nullptr;
  }

  const Request* ret = crawl_queue_.front();
  crawl_queue_.pop();
  return ret;
}

const Request* DataStore::GetRequestFromProbeQueue() {
  absl::MutexLock l(&probe_queue_mutex_);
  if (probe_queue_.empty()) {
    return nullptr;
  }

  const Request* ret = probe_queue_.front();
  probe_queue_.pop();
  return ret;
}

void DataStore::Report(
    const std::vector<std::unique_ptr<ReporterInterface>>& reporters) {
  int initial_depth = 1;
  absl::MutexLock l(&site_pivots_mutex_);
  for (const auto& iter : site_pivots_) {
    for (const auto& reporter : reporters) {
      reporter->ReportPivot(iter.second.get(), initial_depth);
      iter.second->Report(reporter.get(), initial_depth);
    }
  }
}

}  // namespace plusfish
