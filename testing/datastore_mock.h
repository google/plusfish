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

#ifndef PLUSFISH_TESTING_DATASTORE_MOCK_H_
#define PLUSFISH_TESTING_DATASTORE_MOCK_H_

#include <functional>
#include <memory>
#include <vector>

#include "gmock/gmock.h"
#include "datastore.h"

namespace plusfish {

class Request;
class ReporterInterface;

namespace testing {

// Mock for the DataStore class.
class MockDataStore : public DataStore {
 public:
  MOCK_METHOD(size_t, audit_queue_size, (), (override));
  MOCK_METHOD(size_t, probe_queue_size, (), (override));
  MOCK_METHOD(size_t, crawl_queue_size, (), (override));
  MOCK_METHOD(const Request*, GetRequestFromAuditQueue, (), (override));
  MOCK_METHOD(const Request*, GetRequestFromProbeQueue, (), (override));
  MOCK_METHOD(const Request*, GetRequestFromCrawlQueue, (), (override));
  MOCK_METHOD(void, AddRequestToAuditQueue, (const Request*), (override));
  MOCK_METHOD(void, AddRequestToCrawlQueue, (const Request*), (override));
  MOCK_METHOD(void, AddHost, (const std::string& domain_or_ip), (override));
  MOCK_METHOD(void, SetCrawlerCallback,
              (std::function<bool(Request*)> callback));
  MOCK_METHOD(
      void, Report,
      (const std::vector<std::unique_ptr<ReporterInterface>>& reporters),
      (override));
  MOCK_METHOD(bool, AddRequestMetadata,
              (const int64 request_id, const MetaData_Type type,
               const int64 value),
              (override));
  MOCK_METHOD(bool, GetRequestMetadata,
              (const int64 request_id, const MetaData_Type type, int64* value),
              (override));
  MOCK_METHOD(Request*, GetRequestById, (int64 id), (override));
  MOCK_METHOD(bool, AddResponseFingerprintToRequest,
              (const int64 request_id,
               std::unique_ptr<HtmlFingerprint> fingerprint),
              (override));
  MOCK_METHOD(bool, IsFileNotFoundHtmlFingerprint,
              (const HtmlFingerprint* fingerprint), (override));
};

}  // namespace testing
}  // namespace plusfish

#endif  // PLUSFISH_TESTING_DATASTORE_MOCK_H_
