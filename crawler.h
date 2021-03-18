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

#ifndef PLUSFISH_CRAWLER_H_
#define PLUSFISH_CRAWLER_H_

#include <map>
#include <memory>
#include <unordered_set>

#include "audit/passive_auditor.h"
#include "request_handler.h"

namespace plusfish {

class DataStore;
class HttpClientInterface;
class Request;
class SelectiveAuditor;

// The crawler class takes requests, uses the http_client to fetch the content.
// It then extracts links and repeats. Sample usage:
//   Crawler crawl(new HttpClient());
//   crawl.Crawl(new Request("http://www.google.com"));
class Crawler : public RequestHandlerInterface {
 public:
  Crawler();
  // Does not take ownership.
  Crawler(HttpClientInterface* http_client, DataStore* datastore);
  // Does not take ownership. Injecting auditors here is optional and will cause
  // the crawler to hand off completed Requests to them.
  Crawler(HttpClientInterface* client, SelectiveAuditor* selective_auditor,
          PassiveAuditor* passive_auditor, DataStore* datastore);
  ~Crawler() override;

  // Schedule the given request for crawling. Returns true when the request was
  // successfully scheduled with the HTTP client. Else false is returned.
  // Does not take ownership.
  virtual bool Crawl(Request* req);

  // The callback for handling response data parses the response. It
  // extracts links and schedules new requests.
  // Does not take ownership of the request.
  int RequestCallback(Request* req) override;

  // Scrape information from the Request.
  // Returns true if the request was scraped. Else false.
  // Does not take ownership.
  virtual bool Scrape(const Request* req);
  // Same as above but also extracts links and feeds them to the datastore.
  virtual bool ScrapeWithLinks(const Request* req);

 private:
  // Scrape information from the Request. If extract_links is true, links will
  // be extracted and fed to the datastore. If store_fingerprint is true then
  // the response fingerprint is stored in the request which is not necessary
  // for actual test requests..
  // Returns true if the request was scraped. Else false.
  // Does not take ownership.
  bool ScrapeRequest(const Request* req, bool extract_links,
                     bool store_fingerprint);
  // Add a new URL to the datastore. Unless it's a plusfish injected URL in
  // which case this will add an IssueDetails entry to the datastore.
  // Returns true on success. Else false.
  // Does not take ownership.
  bool MaybeAddURL(const Request* ref_req, const std::string& url);
  // The security auditor instance. Optional and not owned.
  SelectiveAuditor* selective_auditor_;
  // The passive auditor instance. Optional and not owned.
  PassiveAuditor* passive_auditor_;
  // The HTTP client instance. Not owned.
  HttpClientInterface* http_client_;
  // The datastore containing all requests. Not owned.
  DataStore* datastore_;
};

}  // namespace plusfish

#endif  // PLUSFISH_CRAWLER_H_
