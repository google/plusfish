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

#ifndef PLUSFISH_TESTING_CURL_MOCK_H_
#define PLUSFISH_TESTING_CURL_MOCK_H_

#include "gmock/gmock.h"
#include "curl.h"

namespace plusfish {
namespace testing {

// The curl mock which can be injected in the CurlHttpClient class to
// simulate curl library calls.
class MockCurl : public Curl {
 public:
  MOCK_CONST_METHOD2(MultiAddHandle,
                     const CURLMcode(CURLM* multi_handle, CURL* easy_handle));
  MOCK_CONST_METHOD1(MultiCleanup, const CURLMcode(CURLM* multi_handle));
  MOCK_CONST_METHOD2(MultiInfoRead,
                     CURLMsg*(CURLM* multi_handle, int* msgs_in_queue));
  MOCK_CONST_METHOD0(MultiInit, CURLM*());
  MOCK_CONST_METHOD2(MultiPerform, const CURLMcode(CURLM* multi_handle,
                                                   int* running_handles));
  MOCK_CONST_METHOD3(MultiSetOpt,
                     const CURLMcode(CURLM* multi_handle, CURLMoption option,
                                     const void* param));
  MOCK_CONST_METHOD1(MultiEnablePipelining,
                     const CURLMcode(CURLM* multi_handle));
  MOCK_CONST_METHOD1(GetTimeout, const int(CURLM* multi_handle));
  MOCK_METHOD2(Select, const int(CURLM* multi_handle, struct timeval* timeout));
  MOCK_METHOD1(EasyCleanup, void(CURL* handle));
  MOCK_CONST_METHOD0(EasyInit, CURL*());
  MOCK_CONST_METHOD3(EasyGetInfo,
                     const CURLcode(CURL* curl, CURLINFO info, void* param));
  MOCK_CONST_METHOD3(EasySetOpt, const CURLcode(CURL* handle, CURLoption option,
                                                const void* param));
  MOCK_CONST_METHOD3(EasySetOptInt64,
                     const CURLcode(CURL* handle, CURLoption option,
                                    const int64 param));
  MOCK_CONST_METHOD3(EasySetWriteCallback,
                     const CURLcode(CURL* handle, CURLoption option,
                                    WriteCallback callback));
  MOCK_CONST_METHOD1(EasyGetHandleData, CurlHandleData*(CURL* handle));
  MOCK_CONST_METHOD2(AppendSlist,
                     bool(struct curl_slist** list, const std::string& value));
  MOCK_CONST_METHOD1(FreeSlist, void(struct curl_slist* list));
};

}  // namespace testing
}  // namespace plusfish

#endif  // PLUSFISH_TESTING_CURL_MOCK_H_
