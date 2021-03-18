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

#ifndef OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_CURL_HTTP_CLIENT_H_
#define OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_CURL_HTTP_CLIENT_H_

#include <memory>
#include <queue>

#include "base/integral_types.h"
#include "base/macros.h"
#include "absl/base/thread_annotations.h"
#include "absl/container/node_hash_map.h"
#include "absl/container/node_hash_set.h"
#include "absl/synchronization/mutex.h"
#include <curl/curl.h>
#include "curl.h"
#include "http_client.h"
#include "request.h"
#include "util/curl_util.h"
#include "util/ratelimiter.h"

namespace plusfish {

// Curl library callback that handles HTTP response data.
// The buffer contains the response data Curl has received. The size of the
// buffer needs to be calculated by multiplying 'size' with 'multiplier'.
// A Request instance is given via the ptr parameter.
// See: http://curl.haxx.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
// Returns the number of bytes read from the buffer. Depending on whether the
// the buffer can still be read into the Request; the size of the buffer or 0 is
// returned.
// Does not take ownership.
// TODO: Support partial updates.
size_t CurlCallback(const char* buffer, const size_t size,
                    const size_t multiplier, void* ptr);

class CurlHttpClient : public HttpClientInterface {
 public:
  // Instantiates a client with the given request per second rate limit.
  explicit CurlHttpClient(int max_request_rate_sec);

  // Instantiates a client for testing purpose.
  // Takes ownership of both parameters which cannot be NULL.
  CurlHttpClient(Curl* curl, RateLimiterInterface* rate_limiter);
  ~CurlHttpClient() override;

  // Return the schedule queue size. This queue contains curl handles for
  // requests that still have to be scheduled for fetching.
  size_t schedule_queue_size() override {
    absl::ReaderMutexLock r(&schedule_queue_mutex_);
    return schedule_queue_.size();
  }

  // Return the number of running handles.
  uint32 running_handles_size() {
    absl::ReaderMutexLock r(&running_handles_mutex_);
    return running_handles_.size();
  }

  size_t active_requests_count() override { return running_handles_size(); }

  // Returns whether or not the client is enabled.
  bool enabled() override;

  // Returns the total amount of HTTP requests that have been performed by the
  // client.
  size_t requests_performed_count() override { return requests_performed_; }

  // Initialize internal properties. Returns True on successful initialisation
  // and False on a failure.
  bool Initialize() ABSL_MUST_USE_RESULT;

  // Schedule a single request. The request is expected to include a
  // RequestHandler instance for processing the response.
  // Inherited. Doesn't take ownership.
  bool Schedule(Request* req) override;
  // Schedule a single request and set the given RequestHandler for processing
  // the result. Basically wraps the Schedule(Request) method.
  // Inherited. Doesn't take ownership.
  bool Schedule(Request* req, RequestHandlerInterface* rh) override;

  // Set a custom header on requests that match the given domain. If the domain
  // matches the wildcard string the header will be set on _all_ requests.
  // Returns true on success and false on failure.
  bool RegisterDefaultHeader(const std::string& domain, const std::string& name,
                             const std::string& value) override;

  // Start new requests and deal with the necessary I/O. Returns the remaining
  // (outstanding) request count.
  // Inherited.
  int Poll() override;

  // Disable the client. This will cause all new Schedule requests to be
  // rejected. Poll() can still be called to let all existing requests finish.
  void Disable() override;

  // Enable the client.
  void Enable() override;

  // Takes scheduled requests (curl easy handlers) from the schedule queue and
  // adds them to the multi handle. The amount of requests added to the handle
  // depends on the maximum allowed connections and the amount of handles
  // that are already running.
  // Returns true if new requests were scheduled. Else false.
  // Inherited.
  bool StartNewRequests() override;
  // Returns a new Curl handle. Used for every new request.
  // Does not take ownership.
  CurlHandleDataPtr NewHandle(Request* req) const;

 private:
  // Sets request handling timestamps in the given request.
  // These timestamps are read from the curl handle itself.
  // True is returned if they could be read; else false is returned.
  // Does not take ownership.
  bool SetRequestTimestamps(CURL* handle, Request* req);

  // Sets the headers from the HTTP request in the CurlHandleData struct.
  bool SetRequestHeaders(CurlHandleData* handle_data, const Request* req);

  // Cleanup a curl handle and it's data. Returns true on success and false on
  // failure.
  bool CleanupCurlHandleData(CurlHandleDataPtr handle_data);

  // The libcurl wrapper object.
  std::unique_ptr<Curl> curl_;
  // The request rate limiter.
  std::unique_ptr<RateLimiterInterface> rate_limiter_;
  // The main (multi) handle used for async Curl requests.
  CURLM* multi_handle_;
  // Object for sharing data (e.g. cookies) between Curl handles.
  CURLSH* curl_share_;
  // Whether the client is enabled.
  bool enabled_ ABSL_GUARDED_BY(enabled_mutex_);
  // Mutex for the enabled flag.
  absl::Mutex enabled_mutex_;
  // Simple counter for in-flight requests.
  int curl_handle_cnt_;
  // Whether the client is initialized.
  bool initialized_;
  // The total HTTP requests performed.
  uint64_t requests_performed_;
  // The queue for new requests.
  std::queue<Request*> schedule_queue_ ABSL_GUARDED_BY(schedule_queue_mutex_);
  // Mutex for the running handles set.
  absl::Mutex running_handles_mutex_;
  // The running handles.
  absl::node_hash_set<CurlHandleDataPtr> running_handles_
      ABSL_GUARDED_BY(running_handles_mutex_);
  // Mutex for the queue.
  absl::Mutex schedule_queue_mutex_;
  // The default headers (and their values) which are stored per domain.
  // For example: "www.google.com": {"X-My-Header": "MyValue"}.
  absl::node_hash_map<std::string,
                      absl::node_hash_map<std::string, std::string>>
      default_headers_ ABSL_GUARDED_BY(default_headers_mutex_);
  absl::Mutex default_headers_mutex_;

  DISALLOW_COPY_AND_ASSIGN(CurlHttpClient);
};

}  // namespace plusfish

#endif  // OPS_SECURITY_PENTESTING_SCANNING_PLUSFISH_CURL_HTTP_CLIENT_H_
