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

#include "curl.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <curl/curl.h>
#include <glog/logging.h>
#include "base/integral_types.h"
#include "request.h"
#include "util/http_util.h"

namespace plusfish {

// The parameter value to enable libcurl HTTP pipelining.
static const int kEnablePipeliningParam = 1;

// Maximum timeout value.
static const int kMaxTimeoutMs = 3000;

Curl::Curl() {}

Curl::~Curl() {}

const CURLMcode Curl::MultiAddHandle(CURLM* multi_handle,
                                     CURL* easy_handle) const {
  return curl_multi_add_handle(multi_handle, easy_handle);
}

CURLMsg* Curl::MultiInfoRead(CURLM* multi_handle, int* msgs_in_queue) const {
  return curl_multi_info_read(multi_handle, msgs_in_queue);
}

CURLM* Curl::MultiInit() const { return curl_multi_init(); }

const CURLMcode Curl::MultiCleanup(CURLM* multi_handle) const {
  return curl_multi_cleanup(multi_handle);
}

const CURLMcode Curl::MultiEnablePipelining(CURLM* multi_handle) const {
  return curl_multi_setopt(multi_handle, CURLMOPT_PIPELINING,
                           &kEnablePipeliningParam);
}

const CURLMcode Curl::MultiPerform(CURLM* multi_handle,
                                   int* running_handles) const {
  return curl_multi_perform(multi_handle, running_handles);
}

const CURLMcode Curl::MultiSetOpt(CURLM* multi_handle, CURLMoption option,
                                  const void* param) const {
  return curl_multi_setopt(multi_handle, option, param);
}

const int Curl::GetTimeout(CURLM* multi_handle) const {
  // The timeout value is set by the curl_multi_timeout call.
  long timeout = -1;  // NOLINT(runtime/int)
  curl_multi_timeout(multi_handle, &timeout);
  if (timeout > kMaxTimeoutMs) {
    return kMaxTimeoutMs;
  }

  return static_cast<int>(timeout);
}

const int Curl::Select(CURLM* multi_handle, struct timeval* timeout) {
  int maxfd_;
  fd_set fdread_;
  fd_set fdwrite_;
  fd_set fdexcep_;

  FD_ZERO(&fdread_);
  FD_ZERO(&fdwrite_);
  FD_ZERO(&fdexcep_);

  curl_multi_fdset(multi_handle, &fdread_, &fdwrite_, &fdexcep_, &maxfd_);
  if (maxfd_ == -1) {
    return -1;
  }

  return select(maxfd_ + 1, &fdread_, &fdwrite_, &fdexcep_, timeout);
}

void Curl::EasyCleanup(CURL* handle) { return curl_easy_cleanup(handle); }

CURL* Curl::EasyInit() const { return curl_easy_init(); }

CurlHandleData* Curl::EasyGetHandleData(CURL* handle) const {
  CurlHandleData* handle_data = nullptr;
  curl_easy_getinfo(handle, CURLINFO_PRIVATE, &handle_data);
  return handle_data;
}

const CURLcode Curl::EasyGetInfo(CURL* handle, CURLINFO info,
                                 void* param) const {
  return curl_easy_getinfo(handle, info, param);
}

const CURLcode Curl::EasySetOpt(CURL* handle, CURLoption option,
                                const void* param) const {
  return curl_easy_setopt(handle, option, param);
}

const CURLcode Curl::EasySetOptInt64(CURL* handle, CURLoption option,
                                     int64 param) const {
  return curl_easy_setopt(handle, option, param);
}

const CURLcode Curl::EasySetWriteCallback(CURL* handle, CURLoption option,
                                          WriteCallback callback) const {
  return curl_easy_setopt(handle, option, callback);
}

CURLSH* Curl::ShareInit() const { return curl_share_init(); }

const CURLSHcode Curl::ShareData(CURLSH* share, curl_lock_data data) {
  return curl_share_setopt(share, CURLSHOPT_SHARE, data);
}

void Curl::FreeSlist(struct curl_slist* list) const {
  curl_slist_free_all(list);
}

bool Curl::AppendSlist(struct curl_slist** list,
                       const std::string& value) const {
  struct curl_slist* tmp_list = nullptr;
  tmp_list = curl_slist_append(*list, value.c_str());
  if (tmp_list != nullptr) {
    *list = tmp_list;
    return true;
  }
  return false;
}

}  // namespace plusfish
