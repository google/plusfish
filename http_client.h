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

#ifndef PLUSFISH_HTTP_CLIENT_H_
#define PLUSFISH_HTTP_CLIENT_H_

#include "opensource/deps/base/macros.h"
#include "request.h"
#include "request_handler.h"

namespace plusfish {

// Plusfish can work with different HTTP client implementation. These
// clients need to implement this interface.
// TODO: clarify and document the ownership
//   of the objects passed as pointers to the methods below.
class HttpClientInterface {
 public:
  virtual ~HttpClientInterface() {}
  // Returns true if this client is enabled.
  virtual bool enabled() = 0;
  // Returns the amount of requests that are scheduled for fetching, but are not
  // on the wire yet.
  virtual size_t schedule_queue_size() = 0;
  // Returned the total requests performed.
  virtual size_t requests_performed_count() = 0;
  // Returns the amount of requests that are currently active
  virtual size_t active_requests_count() = 0;
  // Schedules a request for fetching.
  // Returns true if the request could be scheduled successfully.
  virtual bool Schedule(Request* req) = 0;
  // Schedules a request for fetching, and uses the given RequestHandler
  // to process the server response.  Returns true if scheduling succeeded.
  virtual bool Schedule(Request* req, RequestHandlerInterface* rh) = 0;
  // Polls all scheduled requests and return the number of remaining requests.
  // The implementation is expected to handle requests (e.g. read
  // response) and immediately return.
  virtual int Poll() = 0;
  // Takes scheduled requests and handles (e.g. runs) them.
  // Returns true if new requests were started. Else false.
  virtual bool StartNewRequests() = 0;

  // Sets a custom header on all requests to the given domain.
  // Returns true on success and false on failure.
  virtual bool RegisterDefaultHeader(const std::string& domain,
                                     const std::string& name,
                                     const std::string& value) = 0;
  // Enables the client.
  virtual void Enable() = 0;
  // Disables the client.  The client should stop scheduling new requests.
  virtual void Disable() = 0;

 protected:
  HttpClientInterface() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(HttpClientInterface);
};

}  // namespace plusfish

#endif  // PLUSFISH_HTTP_CLIENT_H_
