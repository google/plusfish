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

#include "request.h"

#include <glog/logging.h>

#include <ctime>
#include <unordered_set>

#include "absl/container/node_hash_set.h"
#include "absl/flags/flag.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "audit/security_check.h"
#include "datastore.h"
#include "opensource/deps/base/integral_types.h"
#include "proto/issue_details.pb.h"
#include "request_handler.h"
#include "util/http_util.h"

using absl::SkipEmpty;

ABSL_FLAG(std::string, user_agent, "Plusfish",
          "The default HTTP user-agent value.");

namespace {
// Helper method to compare the names of repeated RequestField's. Returns true
// when given fields have the exact same set of names. Ignores the field
// values and order.
static bool FieldNamesEqual(const google::protobuf::RepeatedPtrField<
                                plusfish::HttpRequest_RequestField>& one,
                            const google::protobuf::RepeatedPtrField<
                                plusfish::HttpRequest_RequestField>& two) {
  if (one.size() != two.size()) {
    return false;
  }
  absl::node_hash_set<std::string> unique_names;
  for (const auto& field : one) {
    unique_names.emplace(field.name());
  }
  for (const auto& field : two) {
    unique_names.emplace(field.name());
  }
  return unique_names.size() == one.size();
}
}  // namespace

namespace plusfish {

// The max number of elements when splitting a parameter key/value pair.
static const int kParamPairSize = 2;
// The default HTTP scheme
static const char* kSchemeHTTP = "http";
// The HTTPS scheme
static const char* kSchemeHTTPS = "https";

Request::Request()
    : id_(DataStore::kInvalidId),
      parent_id_(DataStore::kInvalidId),
      request_handler_(nullptr),
      origin_(nullptr) {}

Request::Request(const std::string& url, const Request* origin)
    : id_(DataStore::kInvalidId),
      parent_id_(DataStore::kInvalidId),
      request_handler_(nullptr),
      origin_(origin) {
  ParseUrl(url, origin);
  DLOG(INFO) << "Created Request for: " << url;
}

Request::Request(const std::string& url)
    : id_(DataStore::kInvalidId),
      parent_id_(DataStore::kInvalidId),
      request_handler_(nullptr),
      origin_(nullptr) {
  ParseUrl(url, NULL);
  DLOG(INFO) << "Created Request for: " << url;
}

Request::Request(const HttpRequest& proto)
    : id_(DataStore::kInvalidId),
      parent_id_(DataStore::kInvalidId),
      request_(proto),
      origin_(nullptr) {
  google_url_.reset(new GURL(build_url()));
  DLOG(INFO) << "Created Request from proto: " << proto.DebugString();
  build_url();
}

Request::~Request() { DLOG(INFO) << "Request destroyed: " << url_; }

void Request::set_request_handler(RequestHandlerInterface* rh) {
  request_handler_ = rh;
}

void Request::set_ip(const std::string& ip) { request_.set_ip(ip); }

void Request::ResponseCb(const char* data, int size) {
  raw_response_.append(data, size);
}

const std::string& Request::scheme() {
  if (scheme_.empty()) {
    if (request_.ssl()) {
      scheme_ = kSchemeHTTPS;
    } else {
      scheme_ = kSchemeHTTP;
    }
  }
  return scheme_;
}

const std::string& Request::host() const {
  return request_.has_host() ? request_.host() : request_.ip();
}

std::string Request::path() const {
  std::vector<std::string> path_segments;
  for (const auto& segment : request_.path()) {
    path_segments.push_back(segment.name());
    path_segments.push_back(segment.value());
  }
  return absl::StrJoin(path_segments, "");
}

const std::string& Request::build_url() {
  url_ = scheme() + "://" + request_.host();
  url_.append(":" + std::to_string(request_.port()));
  // Append the path.
  url_.append(path());

  // Build the parameters.
  std::string tmp;
  if (request_.param_size()) {
    for (const auto& param : request_.param()) {
      tmp.empty() ? tmp.append("?") : tmp.append("&");
      if (param.has_value()) {
        tmp.append(param.name() + "=" + param.value());
      } else {
        // The "/?debug" parameter scenario.
        tmp.append(param.name());
      }
    }
  }
  url_.append(tmp);

  // TODO: Parse URL fragments and use those instead of raw_fragments.
  if (request_.has_raw_fragment()) {
    url_.append(request_.raw_fragment());
  }
  request_.set_url(url_);
  PrepareHeaders();
  return url_;
}

void Request::PrepareHeaders() {
  SetHeader(HTTPHeaders::kUserAgent, absl::GetFlag(FLAGS_user_agent),
            false /* replace */);
  // TODO: Implement full header control so that other headers (such
  // as Accept, Content-type) can be set here as well.
}

bool Request::ParseUrl(const std::string& url, const Request* ref) {
  google_url_.reset(new GURL(url));
  // Parse as relative URL when initial parsing failed.
  if (!google_url_->is_valid() && (ref && ref->url_is_valid())) {
    *google_url_ = ref->gurl()->Resolve(url);
  }

  // Without a valid URL, we cannot continue.
  if (!google_url_->is_valid()) {
    return false;
  }

  request_.set_raw_url(url);
  request_.set_host(google_url_->host());
  request_.set_port(google_url_->EffectiveIntPort());
  request_.set_ssl(google_url_->SchemeIsSecure());

  // Parse the path string.
  std::vector<std::string> path_vector =
      absl::StrSplit(google_url_->path().c_str(), '/', SkipEmpty());
  HttpRequest_RequestField* last_path_element = nullptr;
  for (const auto& path_value : path_vector) {
    last_path_element = request_.add_path();
    last_path_element->set_name("/");
    last_path_element->set_value(path_value);
  }
  // The last path element has no trailing value and it therefore omitted by the
  // split above. If the URL ends with a "/", we add it here.
  if (last_path_element == nullptr || google_url_->path().back() == '/') {
    last_path_element = request_.add_path();
    last_path_element->set_name("/");
    last_path_element->set_value("");
  }

  // Parse the query string.
  if (google_url_->has_query()) {
    std::vector<std::string> query_vector =
        absl::StrSplit(google_url_->query().c_str(), '&');

    for (const auto& query_iter : query_vector) {
      std::vector<std::string> tmp = absl::StrSplit(query_iter.c_str(), '=');

      HttpRequest_RequestField* query_field = request_.add_param();
      query_field->set_name(tmp.at(0));
      if (tmp.size() == kParamPairSize) {
        query_field->set_value(tmp.at(1));
      }
    }
  }
  build_url();
  return true;
}

bool Request::Equals(const Request& ref) const {
  const HttpRequest& ref_request = ref.proto();
  // We cannot rely on the GURL or HttpRequest == comparison because
  // it's not fuzzy enough (e.g. we don't want all values to count). Returns
  // false when the (exact same) parameters are not in the same order.
  if (request_.ssl() == ref_request.ssl() &&
      request_.method() == ref_request.method() &&
      request_.host() == ref_request.host() &&
      request_.port() == ref_request.port() &&
      request_.param_size() == ref_request.param_size() &&
      request_.body_param_size() == ref_request.body_param_size() &&
      request_.path_size() == ref_request.path_size()) {
    // Compare the sorted URL/body parameters.
    if (!FieldNamesEqual(request_.param(), ref_request.param()) ||
        !FieldNamesEqual(request_.body_param(), ref_request.body_param())) {
      return false;
    }

    // Compare the path elements.
    for (int i = 0; i < request_.path_size(); ++i) {
      if (request_.path(i).value() != ref_request.path(i).value()) {
        return false;
      }
    }
    // TODO: add fragment comparison.
    return true;
  }
  return false;
}

int Request::ReplaceOrAddExistingField(
    const std::string& name, const std::string& value, bool replace,
    google::protobuf::RepeatedPtrField<plusfish::HttpRequest_RequestField>*
        fields) {
  if (!fields->empty()) {
    int updated_fields = 0;
    for (auto& field : *fields) {
      if (field.name() == name) {
        // The header is here but we're not asked to replace it: return.
        if (!replace) {
          return 0;
        }
        field.set_value(value);
        ++updated_fields;
      }
    }
    if (updated_fields) {
      return updated_fields;
    }
  }

  // Getting here means the field is missing and can be added.
  auto new_field = fields->Add();
  new_field->set_name(name);
  new_field->set_value(value);
  return 1;
}

int Request::SetGetParameter(const std::string& name, const std::string& value,
                             bool replace) {
  int ret =
      ReplaceOrAddExistingField(name, value, replace, request_.mutable_param());
  build_url();
  return ret;
}

int Request::SetPostParameter(const std::string& name, const std::string& value,
                              bool replace) {
  // Make sure we capture this is a post request.
  request_.set_method(HttpRequest_RequestMethod_POST);
  return ReplaceOrAddExistingField(name, value, replace,
                                   request_.mutable_body_param());
}

int Request::SetHeader(const std::string& name, const std::string& value,
                       bool replace) {
  return ReplaceOrAddExistingField(name, value, replace,
                                   request_.mutable_header());
}

std::string Request::GetRequestBody() const {
  std::string req_body;

  for (const auto& param : request_.body_param()) {
    if (!req_body.empty()) {
      req_body.append("&");
    }
    // TODO: Apply value encoding here.
    req_body.append(param.name() + "=" + param.value());
  }

  return req_body;
}

void Request::DoneCb() {
  DLOG(INFO) << "Calling request callback for: " << url();
  if (raw_response_.empty()) {
    DLOG(WARNING) << "Request has no response.";
    return;
  }

  response_.reset(new Response());
  if (!response_->Parse(raw_response_)) {
    DLOG(WARNING) << "Unable to parse response for: " << url();
    response_.reset();
    return;
  }
  raw_response_.clear();
  request_handler_->RequestCallback(this);
}

void Request::AddIssue(const IssueDetails* issue) {
  issues_[issue->type()].emplace(issue);
}

}  // namespace plusfish
