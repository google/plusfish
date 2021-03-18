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

#ifndef PLUSFISH_UTIL_CURL_UTIL_H_
#define PLUSFISH_UTIL_CURL_UTIL_H_

#include <memory>

#include <curl/curl.h>
#include "request.h"

namespace plusfish {

// A struct to wrap the libcurl handle and it's plusfish objects.
struct CurlHandleData {
  CURL* easy_handle;               // The libcurl easy handle.
  struct curl_slist* header_list;  // The libcurl header list.

  // The Request instance which, unlike the other objects, is not owned
  // by the struct.
  Request* request;
};

// Wrapper for cleaning up Curl handle data structs.
class CurlHandleDataCloser {
 public:
  void operator()(CurlHandleData* handle_data) {
    if (handle_data == nullptr) {
      return;
    }

    if (handle_data->easy_handle != nullptr) {
      curl_easy_cleanup(handle_data->easy_handle);
    }

    if (handle_data->header_list != nullptr) {
      curl_slist_free_all(handle_data->header_list);
    }
    delete handle_data;
  }
};

typedef std::unique_ptr<CurlHandleData, CurlHandleDataCloser> CurlHandleDataPtr;

}  // namespace plusfish

#endif  // PLUSFISH_UTIL_CURL_UTIL_H_
