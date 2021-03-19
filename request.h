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

#ifndef PLUSFISH_REQUEST_H_
#define PLUSFISH_REQUEST_H_

#include <stdio.h>
#include <stdlib.h>

#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "absl/container/node_hash_set.h"
#include "opensource/deps/base/integral_types.h"
#include "opensource/deps/base/macros.h"
#include "proto/http_request.pb.h"
#include "proto/issue_details.pb.h"
#include "request_handler.h"
#include "response.h"
#include "src/gurl.h"
#include "util/html_fingerprint.h"

namespace plusfish {

class SecurityCheckInterface;

// The message used to replace unwanted responses
static const char* const kTruncatedResponseMessage = "[truncated by plusfish]";

// Represent an HTTP request which is parsed from a std::string (later also from
// a protobuf). The parsed request is stored internally as a protobuf.
//
// When a request is passed to another class, such as the crawler, it will be
// given a request_handler. The request handler, which then is embedded in the
// request, is then called whenever the HTTP request is used.
//
// Example usage:
//    Request request("http://www.example.org");
//    request.set_request_handler(MyCrawler);
//    request.ResponseCb("<html>", 6);
//    request.DoneCb()
//    delete request;
//
// Example usage for relative URLs:
//    Request request("/foo", referring_request);
//    request.set_request_handler(MyCrawler);
//    request.ResponseCb("<html>", 6);
//    request.DoneCb()
//    delete request;
//
// Note that DoneCb() will call a request handler method. This method
// will be given a reference to the request instance and allows the handler
// to access both the HTTP request and response bits.
class Request {
 public:
  virtual ~Request();
  Request();

  // Initialize the request with a URL and an origin/reference request.
  // The origin request is especially useful when the given URL is
  // relative (e.g. '/foo'). The URL will be parsed into the encapsulated
  // HttpRequest proto.
  // Does not own the origin request.
  Request(const std::string& url, const Request* origin);

  // Initialize the request with a URL which will then be parsed into the
  // encapsulated HttpRequest proto. This should never be a relative URL.
  explicit Request(const std::string& url);

  // Create a new Request based on an existing HttpRequest proto.
  explicit Request(const HttpRequest& proto);

  // Set the request ID.
  void set_id(int64 id) { id_ = id; }

  // Return the request ID.
  const int64 id() const { return id_; }

  // Set the request parent ID.
  void set_parent_id(int64 id) { parent_id_ = id; }

  // Return the request parent ID.
  virtual const int64 parent_id() const { return parent_id_; }

  // Set the request http client completion time.
  void set_client_completion_time_ms(int64 time_ms) {
    request_.set_client_completion_time_ms(time_ms);
  }
  // Get the request http client completion time.
  const int64 client_completion_time_ms() const {
    return request_.client_completion_time_ms();
  }

  // Set the total request time.
  void set_client_time_total_usec(int64 time_usec) {
    request_.set_client_time_total_usec(time_usec);
  }
  // Get the total request time.
  const int64 client_time_total_usec() const {
    return request_.client_time_total_usec();
  }

  // Set the application request time.
  void set_client_time_application_usec(int64 time_usec) {
    request_.set_client_time_application_usec(time_usec);
  }
  // Get the application time.
  const int64 client_time_application_usec() const {
    return request_.client_time_application_usec();
  }

  // Returns a Google URL object representation of the request URL.
  const GURL* gurl() const { return google_url_.get(); }

  // Returns a std::string representation of the proto URL.
  const std::string& url() const { return url_; }

  // Returns a protobuf representation of the request.
  const HttpRequest& proto() const { return request_; }

  // Returns the response object, if present. Else nullptr is returned.
  virtual const Response* response() const { return response_.get(); }

  // Truncate the response body.
  virtual void truncate_response_body() {
    return response_->set_body(kTruncatedResponseMessage);
  }

  // Set the response HTML fingerprint.
  virtual void set_response_html_fingerprint(
      std::unique_ptr<HtmlFingerprint> fingerprint) {
    response_->set_html_fingerprint(std::move(fingerprint));
  }

  // Returns the response string.
  const std::string& raw_response() const { return raw_response_; }

  // Returns the origin Request.
  const Request* origin() const { return origin_; }

  // Return the map with issues detected. Entries are stored per issue type.
  const std::map<const IssueDetails::IssueType,
                 absl::node_hash_set<const IssueDetails*>>&
  issues() const {
    return issues_;
  }

  // Returns a bool that indicates whether the request has a valid URL
  bool url_is_valid() const { return google_url_ && google_url_->is_valid(); }

  // Return the port.
  int port() const { return request_.port(); }

  // Returns the correct URL scheme that applies to this request.
  const std::string& scheme();

  // Return the host. If no host is set, this will return the IP. The value is
  // expected to be used in either URL building or for setting the Host: header.
  const std::string& host() const;

  // Return the URL path. The returned std::string is constructed from the path
  // fields in the request protobuf.
  std::string path() const;

  // Create a request body. Will construct the POST data std::string from the
  // related proto fields.
  std::string GetRequestBody() const;

  // Set the IP to which the request was sent.
  void set_ip(const std::string& ip);

  // Set a request parameter. By default, this will just add the parameter.
  // When 'replace' is true: the function will overwrite existing
  // parameters that match the given name or add the parameter.
  // The caller is responsible for encoding the value properly.
  // Returns the number of updated parameters.
  int SetGetParameter(const std::string& name, const std::string& value,
                      bool replace);

  // Same as the SetParameter method except that it will store the parameter for
  // usage in the HTTP request body (e.g. for a POST request).
  // Returns the number of updated parameters.
  int SetPostParameter(const std::string& name, const std::string& value,
                       bool replace);

  // Set an HTTP header on the request. If headers with this name are
  // present and replace is True: the header values are replaced. Else the
  // header is just added. Any values given here will overwrite global
  // defaults (e.g. User-Agent) for this specific request.
  // Returns the number of headers updated.
  int SetHeader(const std::string& name, const std::string& value,
                bool replace);

  // The request handler is the object that does something meaningful with the
  // request and it's resulting response. Whenever DoneCb() is called, the
  // request handler is given a reference to the request (and typically, the
  // embedded response).
  // This method does not take ownership of the pointer.
  void set_request_handler(RequestHandlerInterface* rh);

  // HTTPClient callback function function that sets response data. This
  // might get called multiple times and so data needs to be buffered.
  // The data is not necessarily NULL terminated and therefore size must be
  // used.
  void ResponseCb(const char* data, int size);

  // HTTPClient callback to indicate that it's done with the request (i.e. full
  // response is received). In this function, a request handler function is
  // called with a reference to the request itself.
  virtual void DoneCb();

  // Allows one request to be compared with another. A request is equal
  // when the URL, minus parameter values, matches.
  bool Equals(const Request& ref) const;

  // Parses a URL, populates the Request proto attributes and return true on
  // success and false on failure. In all cases, it is preferred that a
  // reference URL is provided - as this helps to resolve relative URLs.
  // If a reference URL is not known, a nullptr can be given instead.
  // Does not take ownership of the given reference request.
  bool ParseUrl(const std::string& url, const Request* ref);

  // Add an issue to the current request.
  // Does not take ownership.
  void AddIssue(const IssueDetails* issue);

 private:
  // Returns a URL representation of the request. The URL will be build from the
  // protobuf that is embedded in the request and might not be a 100% exact
  // match with URL used to initiate the Request object.
  const std::string& build_url();

  // If replace is set to true: replace existing values of the field 'name' with
  // the given value. If set to false, add the name=value pair.
  // Returns the number of updated fields.
  // Does not take ownership.
  int ReplaceOrAddExistingField(
      const std::string& name, const std::string& value, bool replace,
      google::protobuf::RepeatedPtrField<plusfish::HttpRequest_RequestField>*
          fields);

  // Helper method to set the headers that are appropiate for this request.
  void PrepareHeaders();

  // The request ID which is assigned by the datastore. A value of -1 means the
  // request is not present in the datastore (e.g. temporary requests).
  int64 id_;
  // The parent ID. This identifies the parent request from which the request
  // was cloned from.
  int64 parent_id_;
  // Holds the Google URL instance.
  std::unique_ptr<GURL> google_url_;
  // The URL scheme.
  std::string scheme_;
  // The raw URL string.
  std::string url_;
  // String with the response.
  std::string raw_response_;
  // The final response object.
  std::unique_ptr<Response> response_;
  // The request POST data string.
  std::string post_data_;

  // Request proto message which contains the parsed HTTP request.
  HttpRequest request_;
  // Pointer to the request handler object.
  // The instance is not owned by the Request class.
  RequestHandlerInterface* request_handler_;
  // A pointer to the origin request. This typically is the request who's
  // response had a URL that resulted in the creation of this request (e.g.
  // referrer).
  const Request* origin_;

  // Issues detected for this Request (stored per security test name).
  std::map<const IssueDetails::IssueType,
           absl::node_hash_set<const IssueDetails*>>
      issues_;
};

}  // namespace plusfish

#endif  // PLUSFISH_REQUEST_H_
