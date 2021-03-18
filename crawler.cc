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

#include "crawler.h"

#include <functional>
#include <unordered_set>

#include <glog/logging.h>
#include "absl/container/node_hash_set.h"
#include "absl/flags/flag.h"
#include "audit/selective_auditor.h"
#include "datastore.h"
#include "http_client.h"
#include "parsers/html_scraper.h"
#include "parsers/util/scrape_util.h"
#include "proto/http_common.pb.h"
#include "proto/http_request.pb.h"
#include "proto/http_response.pb.h"
#include "request.h"
#include "request_handler.h"
#include "response.h"
#include "util/http_util.h"

ABSL_FLAG(bool, extract_links, true,
          "Whether the crawler should extract links from the HTTP response.");
ABSL_FLAG(bool, discard_binary_responses, true,
          "Whether binary (including images) responses should be discarded.");

using std::placeholders::_1;

namespace plusfish {

// Archives, images other non-ascii mime types for which we optionally discard
// the response.
static const absl::node_hash_set<MimeInfo::MimeType>& GetBinaryMimeTypes() {
  static absl::node_hash_set<MimeInfo::MimeType>* binary_mime_types = []() {
    return new absl::node_hash_set<MimeInfo::MimeType>(
        {MimeInfo::BIN_CAB, MimeInfo::BIN_ZIP, MimeInfo::BIN_GZIP,
         MimeInfo::IMG_GIF, MimeInfo::IMG_ANI, MimeInfo::IMG_BMP,
         MimeInfo::IMG_PNG, MimeInfo::IMG_TIFF, MimeInfo::EXT_JAR,
         MimeInfo::EXT_PDF, MimeInfo::EXT_CLASS});
  }();
  return *binary_mime_types;
}

// Response codes to not perform security tests against.
static const absl::node_hash_set<HttpResponse::ResponseCode>&
GetNoTestingResponseCodes() {
  static const absl::node_hash_set<HttpResponse::ResponseCode>*
      no_testing_response_codes = []() {
        return new absl::node_hash_set<HttpResponse::ResponseCode>(
            {HttpResponse::NOT_FOUND, HttpResponse::FORBIDDEN,
             HttpResponse::BAD_REQUEST, HttpResponse::METHOD_NOT_ALLOWED,
             HttpResponse::PROXY_AUTHENTICATION_REQUIRED});
      }();
  return *no_testing_response_codes;
}

Crawler::Crawler()
    : selective_auditor_(nullptr), http_client_(nullptr), datastore_(nullptr) {}

Crawler::Crawler(HttpClientInterface* client, DataStore* datastore)
    : selective_auditor_(nullptr),
      passive_auditor_(nullptr),
      http_client_(client),
      datastore_(datastore) {}

Crawler::Crawler(HttpClientInterface* client,
                 SelectiveAuditor* selective_auditor,
                 PassiveAuditor* passive_auditor, DataStore* datastore)
    : selective_auditor_(selective_auditor),
      passive_auditor_(passive_auditor),
      http_client_(client),
      datastore_(datastore) {
  if (selective_auditor_) {
    selective_auditor_->SetCrawlerScrapeCallback(
        std::bind(&Crawler::Scrape, this, _1));
  }
}

Crawler::~Crawler() { DLOG(INFO) << "Crawler stopped."; }

bool Crawler::Crawl(Request* req) {
  // Schedule for initial fetch.
  DLOG(INFO) << "Scheduling request for: " << req->url();
  req->set_request_handler(this);
  return http_client_->Schedule(req);
}

int Crawler::RequestCallback(Request* req) {
  DLOG(INFO) << "Parsing response for: " + req->url();
  // Return immediately if there is no response.
  if (req->response() == nullptr) {
    return 1;
  }

  // Prevent crawler loops by checking the parent and parent parent requests to
  // see if the content is the same.
  if (req->parent_id()) {
    Request* parent = datastore_->GetRequestById(req->parent_id());

    if (parent && parent->response() &&
        parent->response()->Equals(*req->response()) && parent->parent_id()) {
      Request* grandparent = datastore_->GetRequestById(parent->parent_id());
      if (grandparent && grandparent->response() &&
          grandparent->response()->Equals(*req->response())) {
        // The parent parent is the same as well. In this case drop the
        // request to avoid crawler loops.
        DLOG(INFO) << "Preventing crawler loop of URL: " << req->url();
        return 1;
      }
    }
  }

  // TODO: Remove this passive auditor call.
  if (passive_auditor_ != nullptr && !passive_auditor_->Check(req)) {
    DLOG(WARNING) << "Didn't perform passive checks on: " << req->url();
  }

  // Skip testing non-existing files but do take the liberal approach
  // to test all other response codes.
  if (GetNoTestingResponseCodes().contains(req->response()->proto().code()) ||
      datastore_->IsFileNotFoundHtmlFingerprint(
          req->response()->get_html_fingerprint())) {
    DLOG(INFO) << "Not testing page due to HTTP code or 404 fingerprint match: "
               << req->url() << " code "
               << HttpResponse_ResponseCode_Name(
                      req->response()->proto().code());
  } else if (selective_auditor_) {
    datastore_->AddRequestToAuditQueue(req);
  }

  if (!ScrapeWithLinks(req)) {
    DLOG(INFO) << "Didn't scrape: " << req->url();
  }

  if (absl::GetFlag(FLAGS_discard_binary_responses) &&
      GetBinaryMimeTypes().count(req->response()->MimeType())) {
    // Truncate the response.
    req->truncate_response_body();
  }
  return 0;
}  // namespace plusfish

bool Crawler::Scrape(const Request* req) {
  return ScrapeRequest(req, false /* extract links */,
                       false /* store_fingerprint */);
}

bool Crawler::ScrapeWithLinks(const Request* req) {
  // Fingerprinting is used for duplicate detection and crawler loop prevention.
  // When we don't extract links then it's not necessary to store the
  // fingerprint.
  return ScrapeRequest(req, true /* extract links */,
                       true /* store_fingerprint */);
}

bool Crawler::MaybeAddURL(const Request* ref, const std::string& url) {
  // TODO: check if the URL was injected by a test.
  return datastore_->AddRequest(std::unique_ptr<Request>(
             new Request(url, ref))) != DataStore::kInvalidId;
}

// TODO: refactor to improve testing.
bool Crawler::ScrapeRequest(const Request* req, bool extract_links,
                            bool store_fingerprint) {
  // Return when no links have to be extracted.
  bool also_extract_links = absl::GetFlag(FLAGS_extract_links) && extract_links;

  if (!req->response()) {
    DLOG(WARNING) << "Not scraping request without response: " << req->url();
    return false;
  }

  MimeInfo::MimeType mime = req->response()->MimeType();
  // Responses with these mime types below will have their content sniffed
  // (emulate browser sniffing).
  if ((mime == MimeInfo::ASC_GENERIC || mime == MimeInfo::ASC_JAVASCRIPT ||
       mime == MimeInfo::ASC_JSON || mime == MimeInfo::APP_JSON ||
       mime == MimeInfo::APP_JAVASCRIPT || mime == MimeInfo::APP_XJAVASCRIPT) &&
      util::SniffMimeType(req->response()->body(), &mime)) {
    DLOG(INFO) << "Using sniffed mime: " << mime
               << " (old mime: " << req->response()->MimeType() << ")";
  }

  if (mime == MimeInfo::ASC_HTML || mime == MimeInfo::XML_XHTML) {
    HtmlScraper scraper;
    if (!scraper.Parse(req, req->response()->body())) {
      DLOG(WARNING) << "Unable to parse: " << req->response()->body();
      return false;
    }

    if (store_fingerprint && !datastore_->AddResponseFingerprintToRequest(
                                 req->id(), scraper.fingerprint())) {
      DLOG(WARNING) << "Unable to add fingerprint to request id: " << req->id();
    }

    if (also_extract_links) {
      for (const auto& anchor : scraper.anchors()) {
        if (MaybeAddURL(req, anchor)) {
          DLOG(INFO) << "Added scraped URL: " << anchor;
        }
      }
    }

    auto& requests_ = scraper.requests();
    for (auto req_iter = requests_.begin(); req_iter != requests_.end();
         req_iter++) {
      if (datastore_->AddRequest(
              std::unique_ptr<Request>(req_iter->release())) != -1) {
      }
    }

    for (const auto& issue : scraper.issues()) {
      int64 req_id =
          req->id() == DataStore::kInvalidId ? req->parent_id() : req->id();
      std::unique_ptr<IssueDetails> new_issue(new IssueDetails(*issue));
      *new_issue->mutable_request() = req->proto();
      if (req->response() != nullptr) {
        *new_issue->mutable_response() = req->response()->proto();
      }
      datastore_->AddIssueDetails(req_id, std::move(new_issue));
    }
  }

  // Return if no link extraction is requested.
  if (!also_extract_links) {
    return true;
  }

  // Check if there is a Location header value (regardless of the status code).
  const std::string* location_value =
      req->response()->GetHeader(HTTPHeaders::kLocation);
  if (location_value != nullptr) {
    if (MaybeAddURL(req, *location_value)) {
      DLOG(INFO) << "Added scraped location URL: " << location_value;
    }
  }
  return true;
}

}  // namespace plusfish
