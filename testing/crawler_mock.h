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

#ifndef PLUSFISH_TESTING_CRAWLER_MOCK_H_
#define PLUSFISH_TESTING_CRAWLER_MOCK_H_

#include "gmock/gmock.h"
#include "audit/selective_auditor.h"
#include "crawler.h"
#include "datastore.h"

namespace plusfish {
namespace testing {

// Mock for the Crawler class.
class MockCrawler : public Crawler {
 public:
  MockCrawler(HttpClientInterface* client, SelectiveAuditor* active_auditor,
              PassiveAuditor* passive_auditor, DataStore* datastore)
      : Crawler(client, active_auditor, passive_auditor, datastore) {}

  MOCK_METHOD1(Crawl, bool(Request* req));
  MOCK_METHOD1(Scrape, bool(const Request* req));
  MOCK_METHOD1(ScrapeWithLinks, bool(const Request* req));
};

}  // namespace testing
}  // namespace plusfish

#endif  // PLUSFISH_TESTING_CRAWLER_MOCK_H_
