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

#ifndef PLUSFISH_CURL_H_
#define PLUSFISH_CURL_H_

#include <curl/curl.h>
#include "base/integral_types.h"
#include "base/macros.h"

#include "request.h"
#include "util/curl_util.h"

// The callback function (pointer) used by libcurl to return response data.
typedef size_t (*WriteCallback)(const char*, const size_t, const size_t, void*);

namespace plusfish {

// A basic wrapper for the libcurl methods so they don't have to be called
// directly from other classes. Additionally, this makes mocking possible.
class Curl {
 public:
  Curl();
  virtual ~Curl();

  // Adds a handle.
  virtual const CURLMcode MultiAddHandle(CURLM* multi_handle,
                                         CURL* easy_handle) const;
  // Release the handle.
  virtual const CURLMcode MultiCleanup(CURLM* multi_handle) const;
  // Wait for the handles to have data. Returns the number of actionable
  // handle's on success. On error, -1 is returned.
  virtual const int Select(CURLM* multi_handle, struct timeval* timeout);
  // Read information from the multi handle and return a CURLMsg struct.
  // The caller will own the returned instance.
  virtual CURLMsg* MultiInfoRead(CURLM* multi_handle, int* msgs_in_queue) const;
  // Get a multi handle from curl.
  // The caller will own the returned instance.
  virtual CURLM* MultiInit() const;
  // Do I/O on all the handles.
  virtual const CURLMcode MultiPerform(CURLM* multi_handle,
                                       int* running_handles) const;
  // Set an option on the multi handle.
  virtual const CURLMcode MultiSetOpt(CURLM* multi_handle, CURLMoption option,
                                      const void* param) const;
  // A curl multi timeout wrapper. Returns the recommended timeout in
  // milli-seconds.
  virtual const int GetTimeout(CURLM* multi_handle) const;
  // Enable pipelining.
  virtual const CURLMcode MultiEnablePipelining(CURLM* multi_handle) const;
  // Cleanup an easy handle.
  virtual void EasyCleanup(CURL* handle);
  // Create an easy handle.
  // The caller will own the returned handle.
  virtual CURL* EasyInit() const;
  // Get information from the easy handle.
  virtual const CURLcode EasyGetInfo(CURL* curl, CURLINFO info,
                                     void* param) const;
  // Get the CurlHandleData object pointer from the easy handle.
  // Returns null when the object is not present.
  virtual CurlHandleData* EasyGetHandleData(CURL* handle) const;
  // Set an option for a curl easy handle.
  virtual const CURLcode EasySetOpt(CURL* handle, CURLoption option,
                                    const void* param) const;
  // Set an int64 option for a curl easy handle.
  virtual const CURLcode EasySetOptInt64(CURL* handle, CURLoption option,
                                         const int64 param) const;
  // Set the write callback.
  virtual const CURLcode EasySetWriteCallback(CURL* handle, CURLoption option,
                                              WriteCallback callback) const;
  // Get a Curl share object.
  // The caller will own the returned handle.
  virtual CURLSH* ShareInit() const;
  // Enable datasharing via the Curl share object. The data parameter is used to
  // indicate what kind of data to share.
  virtual const CURLSHcode ShareData(CURLSH* share, curl_lock_data data);
  // Append a value to the CURL slist linked list.
  // Use the value format "Header-Name: Header-Value" to set headers.
  // Returns true if the value was appended. If 'list' is a nullptr, it will be
  // initialized and the caller has to take ownership of the list (and release
  // it with FreeSlist).
  virtual bool AppendSlist(struct curl_slist** list,
                           const std::string& value) const;
  // Free Curl slist linked list.
  virtual void FreeSlist(struct curl_slist* list) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(Curl);
};

}  // namespace plusfish

#endif  // PLUSFISH_CURL_H_
