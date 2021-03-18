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

#include "curl_http_client.h"

#include <stdio.h>
#include <unistd.h>

#include <limits>

#include <glog/logging.h>
#include "absl/flags/flag.h"
#include "absl/synchronization/mutex.h"
#include <curl/curl.h>
#include "http_client.h"
#include "proto/http_request.pb.h"
#include "request.h"
#include "util/curl_util.h"
#include "util/ratelimiter.h"
#include "util/simpleratelimiter.h"

ABSL_FLAG(int32_t, connection_limit, 10, "The parallel connection limit");
ABSL_FLAG(int32_t, request_timeout_sec, 10,
          "The maximum duration (in seconds) of a single request.");
ABSL_FLAG(int32_t, max_response_read_size, 512 * 1024,
          "The max bytes to read from the HTTP response. The rest of the "
          "response is discarded.");
ABSL_FLAG(int64_t, response_buffer_size, 256 * 1024,
          "The max size (in bytes) of the buffer curl uses for reading "
          "server responses.");
ABSL_FLAG(std::string, cookie_file, "", "The file used to store cookies");
ABSL_FLAG(std::string, http_proxy, "", "The HTTP proxy to use.");
ABSL_FLAG(std::string, client_ssl_cert, "", "A client SSL certificate (p12).");
ABSL_FLAG(std::string, client_ssl_key, "",
          "The key for the client SSL certificate.");
ABSL_FLAG(std::string, client_ssl_key_password, "",
          "The password for loading the key. It's recommended to only set "
          "this flag via a 'flagsfile'.");

namespace plusfish {

// The minimal timeout for a select call in micro seconds.
static const int kDefaultSelectTimeoutMs = 100000;
// The protocols to enable in libcurl.
static const int kCurlProtocols = CURLPROTO_HTTPS | CURLPROTO_HTTP;
// The Curl option to receive the HTTP headers in the response.
static const int kCurlEnableFullHttpResponse = 1;
// The Curl option for making a Curl POST handle.
static const int kCurlMakePostHandle = 1;
// The ratio of requests to schedule over the maximum allowed connections.
static const float kRequestScheduleRatio = 1.2;
// The string used to represent ALL hostnames.
static const char* kWildcardHostname = "*";
// The option to enable (1) disable (0) curl path cleanup logic. We disable this
// behavior by default because it removes ../ and ./ character sequences from
// our requests and effectively break our tests.
static const int kEnablePathCleanup = 0;

// The curl library callback for handling response data.
size_t CurlCallback(const char* buffer, const size_t size,
                    const size_t multiplier, void* ptr) {
  Request* request = static_cast<Request*>(ptr);
  if (std::numeric_limits<std::size_t>::max() / size < multiplier) {
    LOG(WARNING) << "Libcurl write callback integer overflow prevented.";
    return 0;
  }

  if (size + request->raw_response().length() >
      absl::GetFlag(FLAGS_max_response_read_size)) {
    DLOG(WARNING) << "Skipping remainder of a large response.";
    return 0;
  }

  request->ResponseCb(buffer, size * multiplier);
  return size * multiplier;
}

CurlHttpClient::CurlHttpClient(int max_request_rate_sec)
    : multi_handle_(nullptr),
      curl_share_(nullptr),
      curl_handle_cnt_(0),
      initialized_(false),
      requests_performed_(0) {
  curl_.reset(new Curl());
  rate_limiter_.reset(new SimpleRateLimiter(max_request_rate_sec));
}

CurlHttpClient::CurlHttpClient(Curl* curl, RateLimiterInterface* rate_limiter)
    : multi_handle_(nullptr),
      curl_share_(nullptr),
      curl_handle_cnt_(0),
      initialized_(false),
      requests_performed_(0) {
  CHECK(curl != nullptr);
  CHECK(rate_limiter != nullptr);
  curl_.reset(curl);
  rate_limiter_.reset(rate_limiter);
}

CurlHttpClient::~CurlHttpClient() {
  DLOG(INFO) << "CurlHttpClient destructor - cleaning up";
  if (multi_handle_ != nullptr) {
    if (curl_->MultiCleanup(multi_handle_) != CURLM_OK) {
      LOG(WARNING) << "Unable to cleanup curl multi handle";
    }
  }

  if (curl_share_ != nullptr) {
    if (curl_share_cleanup(curl_share_) != CURLSHE_OK) {
      LOG(WARNING) << "Unable to cleanup curl share object";
    }
  }
}

bool CurlHttpClient::enabled() {
  absl::MutexLock l(&enabled_mutex_);
  return enabled_;
}

void CurlHttpClient::Disable() {
  absl::MutexLock l(&enabled_mutex_);
  enabled_ = false;
}

void CurlHttpClient::Enable() {
  absl::MutexLock l(&enabled_mutex_);
  enabled_ = true;
}

bool CurlHttpClient::Initialize() {
  multi_handle_ = curl_->MultiInit();
  curl_share_ = curl_->ShareInit();
  if (multi_handle_ == nullptr || curl_share_ == nullptr) {
    LOG(WARNING) << "Unable to initialize curl";
    return false;
  }

  if (CURLSHcode ret = curl_->ShareData(curl_share_, CURL_LOCK_DATA_COOKIE)) {
    LOG(WARNING) << "Unable to enable cookie sharing. Error code: " << ret;
    return false;
  }

  const int32_t max_connections = absl::GetFlag(FLAGS_connection_limit);
  if (curl_->MultiSetOpt(multi_handle_, CURLMOPT_MAXCONNECTS,
                         &max_connections) != CURLM_OK) {
    LOG(WARNING) << "Unable to set max connection limit";
    return false;
  }
  Enable();
  initialized_ = true;
  return true;
}

CurlHandleDataPtr CurlHttpClient::NewHandle(Request* req) const {
  DLOG(INFO) << "Creating handle for: " << req->url();

  CurlHandleDataPtr handle_data(new CurlHandleData());
  handle_data->easy_handle = curl_->EasyInit();
  if (handle_data->easy_handle == nullptr ||
      curl_->EasySetOpt(handle_data->easy_handle, CURLOPT_PATH_AS_IS,
                        &kEnablePathCleanup) ||
      curl_->EasySetOpt(handle_data->easy_handle, CURLOPT_URL,
                        req->url().c_str())) {
    LOG(WARNING) << "Unable to create handle from URL: " << req->url();
    return nullptr;
  }

  if (!absl::GetFlag(FLAGS_http_proxy).empty()) {
    LOG(INFO) << "Using HTTP proxy: " << absl::GetFlag(FLAGS_http_proxy);
    if (CURLcode ret =
            curl_->EasySetOpt(handle_data->easy_handle, CURLOPT_PROXY,
                              absl::GetFlag(FLAGS_http_proxy).c_str())) {
      LOG(WARNING) << "Unable to configure the HTTP proxy. Error code: " << ret;
      return nullptr;
    }
  }

  // Load the client certificate.
  if (!absl::GetFlag(FLAGS_client_ssl_cert).empty()) {
    if (absl::GetFlag(FLAGS_client_ssl_key).empty()) {
      LOG(WARNING) << "Please provide an SSL certificate key";
      return nullptr;
    }
    const std::string client_ssl_key_password =
        absl::GetFlag(FLAGS_client_ssl_key_password);
    if (!client_ssl_key_password.empty() &&
        curl_->EasySetOpt(handle_data->easy_handle, CURLOPT_KEYPASSWD,
                          &client_ssl_key_password)) {
      LOG(WARNING) << "Unable to set SSL key password.";
      return nullptr;
    }

    const std::string client_ssl_cert = absl::GetFlag(FLAGS_client_ssl_cert);
    const std::string client_ssl_key = absl::GetFlag(FLAGS_client_ssl_key);
    if (curl_->EasySetOpt(handle_data->easy_handle, CURLOPT_SSLCERT,
                          &client_ssl_cert) ||
        curl_->EasySetOpt(handle_data->easy_handle, CURLOPT_SSLKEY,
                          &client_ssl_key)) {
      LOG(WARNING) << "Unable to load client certificate!";
      return nullptr;
    }
  }

  // Simple (option) changes should never fail and are therefore not
  // tested individually.
  if (curl_->EasySetWriteCallback(handle_data->easy_handle,
                                  CURLOPT_WRITEFUNCTION, CurlCallback) ||
      curl_->EasySetOpt(handle_data->easy_handle, CURLOPT_COOKIEFILE,
                        absl::GetFlag(FLAGS_cookie_file).c_str()) ||
      curl_->EasySetOptInt64(handle_data->easy_handle, CURLOPT_BUFFERSIZE,
                             absl::GetFlag(FLAGS_response_buffer_size)) ||
      curl_->EasySetOptInt64(handle_data->easy_handle, CURLOPT_TIMEOUT,
                             absl::GetFlag(FLAGS_request_timeout_sec)) ||
      curl_->EasySetOptInt64(handle_data->easy_handle, CURLOPT_PROTOCOLS,
                             kCurlProtocols) ||
      // Set the 'share' object which is used to share cookies.
      curl_->EasySetOpt(handle_data->easy_handle, CURLOPT_SHARE, curl_share_) ||
      // Set the response callback.
      curl_->EasySetOpt(handle_data->easy_handle, CURLOPT_WRITEDATA, req) ||
      // Disable SSL verification: for security testing it should not block. We
      // will however report invalid certs later.
      curl_->EasySetOptInt64(handle_data->easy_handle, CURLOPT_SSL_VERIFYPEER,
                             0) ||
      curl_->EasySetOptInt64(handle_data->easy_handle, CURLOPT_SSL_VERIFYHOST,
                             0) ||
      // We want the full headers in the response.
      curl_->EasySetOpt(handle_data->easy_handle, CURLOPT_HEADER,
                        &kCurlEnableFullHttpResponse)) {
    LOG(WARNING) << "Curl setting basic options failed";
    return nullptr;
  }

  // Set the POST body.
  if (req->proto().method() == HttpRequest_RequestMethod_POST) {
    if (curl_->EasySetOpt(handle_data->easy_handle, CURLOPT_COPYPOSTFIELDS,
                          req->GetRequestBody().c_str())) {
      DLOG(WARNING) << "Unable to set POST body for this request.";
      return nullptr;
    }
  }

  return handle_data;
}

bool CurlHttpClient::Schedule(Request* req) {
  DCHECK(initialized_);
  if (!enabled()) {
    return false;
  }

  absl::MutexLock l(&schedule_queue_mutex_);
  schedule_queue_.push(req);
  return true;
}

bool CurlHttpClient::Schedule(Request* req, RequestHandlerInterface* rh) {
  req->set_request_handler(rh);
  return Schedule(req);
}

int CurlHttpClient::Poll() {
  DCHECK(initialized_);
  const CURLMsg* msg;
  struct timeval timeout;

  running_handles_mutex_.ReaderLock();
  if (running_handles_.empty()) {
    DLOG(INFO) << "No handles to poll";
    running_handles_mutex_.ReaderUnlock();
    return 0;
  }
  running_handles_mutex_.ReaderUnlock();

  // Determine the timeout for select.
  int curl_timeout = curl_->GetTimeout(multi_handle_);
  if (curl_timeout > 0) {
    timeout.tv_sec = curl_timeout / 1000;
    timeout.tv_usec = (curl_timeout % 1000) * 1000;
  } else {
    timeout.tv_sec = 0;
    timeout.tv_usec = kDefaultSelectTimeoutMs;
  }

  switch (curl_->Select(multi_handle_, &timeout)) {
    case -1:
      DLOG(WARNING) << "A select error occurred.";
      // Select error: we do nothing.
      break;
    case 0:
      DLOG_EVERY_N(INFO, 5) << "Poll returned nothing";
      return 0;
    default:
      curl_->MultiPerform(multi_handle_, &curl_handle_cnt_);
      break;
  }

  // See how the transfers went. For the completed requests, call the
  // callback method.
  int msgs_left = 0;
  while ((msg = curl_->MultiInfoRead(multi_handle_, &msgs_left))) {
    DLOG(INFO) << "Processing message with state: " << msg->msg
               << " msgs left: " << msgs_left;
    if (msg->msg == CURLMSG_DONE) {
      // Get the Request object.
      CurlHandleDataPtr handle_data(curl_->EasyGetHandleData(msg->easy_handle));
      if (handle_data->request == nullptr) {
        LOG(WARNING) << "Received curl handle without request reference.";
        continue;
      }
      // Capture the IP we're connected to (when no proxy is used).
      if (absl::GetFlag(FLAGS_http_proxy).empty()) {
        char* ip = nullptr;
        if (CURLE_OK == curl_->EasyGetInfo(msg->easy_handle,
                                           CURLINFO_PRIMARY_IP,
                                           static_cast<void*>(&ip)) &&
            ip != nullptr) {
          handle_data->request->set_ip(ip);
        }
      }

      if (!SetRequestTimestamps(msg->easy_handle, handle_data->request)) {
        // This can influence the security testing but is not considered an
        // error condition.
        LOG(WARNING) << "Unable to set request timestamps for: "
                     << handle_data->request->url();
      }

      handle_data->request->DoneCb();
      if (CleanupCurlHandleData(std::move(handle_data)) == false) {
        LOG(WARNING) << "Unable to cleanup handle";
      }
    }
  }

  DLOG(INFO) << "Poll returns (running handles: " << curl_handle_cnt_ << ")";
  return curl_handle_cnt_;
}

bool CurlHttpClient::SetRequestTimestamps(CURL* handle, Request* req) {
  // Underlying libcurl library expects double.
  double connect_time;
  double total_time;
  if (curl_->EasyGetInfo(handle, CURLINFO_TOTAL_TIME, &total_time) ||
      curl_->EasyGetInfo(handle, CURLINFO_CONNECT_TIME, &connect_time)) {
    return false;
  }

  req->set_client_time_application_usec((total_time - connect_time) * 1000000);
  req->set_client_time_total_usec(total_time * 1000000);
  req->set_client_completion_time_ms(time(nullptr) * 1000);
  return true;
}

bool CurlHttpClient::StartNewRequests() {
  if (!enabled()) {
    return false;
  }

  int max_active_handles_allowed =
      (absl::GetFlag(FLAGS_connection_limit) * kRequestScheduleRatio);
  if (curl_handle_cnt_ >= max_active_handles_allowed) {
    DLOG(INFO) << "Amount of running handles exceeds the connection limit"
               << " so not scheduling more.";
    return true;
  }

  int max_to_add = max_active_handles_allowed - curl_handle_cnt_;
  DLOG(INFO) << "Adding " << max_to_add << " connections from the queue";

  absl::MutexLock l(&schedule_queue_mutex_);
  while (!schedule_queue_.empty() && max_to_add > 0 &&
         rate_limiter_->TakeRateSlot()) {
    --max_to_add;
    Request* req = schedule_queue_.front();
    schedule_queue_.pop();

    auto handle_data = NewHandle(req);
    if (!handle_data || handle_data->easy_handle == nullptr) {
      LOG(WARNING) << "Could not add curl handle for URL: " << req->url();
      return false;
    }

    handle_data->request = req;
    handle_data->header_list = nullptr;

    if (!SetRequestHeaders(handle_data.get(), req)) {
      LOG(WARNING) << "Could not set headers on new handle";
      return false;
    }

    // Add a reference to the handle data instance for later use.
    if (curl_->EasySetOpt(handle_data->easy_handle, CURLOPT_PRIVATE,
                          handle_data.get()) != CURLE_OK) {
      LOG(WARNING) << "Could not store private object in handle";
      return false;
    }

    if (curl_->MultiAddHandle(multi_handle_, handle_data->easy_handle) ||
        curl_->MultiPerform(multi_handle_, &curl_handle_cnt_)) {
      // With plusfish, you only get one chance.
      return false;
    }
    ++requests_performed_;
    absl::MutexLock h(&running_handles_mutex_);
    running_handles_.emplace(std::move(handle_data));
  }
  return true;
}

bool CurlHttpClient::SetRequestHeaders(CurlHandleData* handle_data,
                                       const Request* req) {
  if (!req->proto().header_size()) {
    return false;
  }

  // Add the request specific headers.
  for (const auto& header : req->proto().header()) {
    if (!curl_->AppendSlist(&handle_data->header_list,
                            header.name() + ": " + header.value())) {
      LOG(WARNING) << "Unable to set header: " << header.name() << ": "
                   << header.value();
      return false;
    }
  }

  // Add the global (domain specific) headers.
  absl::ReaderMutexLock r(&default_headers_mutex_);
  for (const auto& domain_iter : default_headers_) {
    // If the domain is not a wildcard and doesn't match the domain in the
    // request: skip it.
    if (strcmp(domain_iter.first.c_str(), kWildcardHostname) != 0 &&
        strcasecmp(req->host().c_str(), domain_iter.first.c_str()) != 0) {
      continue;
    }

    for (const auto& header_iter : domain_iter.second) {
      if (!curl_->AppendSlist(&handle_data->header_list,
                              header_iter.first + ": " + header_iter.second)) {
        return false;
      }
    }
  }

  // Set the headers on the curl handle.
  if (curl_->EasySetOpt(handle_data->easy_handle, CURLOPT_HTTPHEADER,
                        handle_data->header_list) != CURLE_OK) {
    LOG(WARNING) << "Unable to set custom headers on curl handle";
    return false;
  }
  return true;
}

bool CurlHttpClient::RegisterDefaultHeader(const std::string& domain,
                                           const std::string& name,
                                           const std::string& value) {
  absl::MutexLock l(&default_headers_mutex_);
  if (name.empty() || value.empty()) {
    return false;
  }

  default_headers_[domain][name] = value;
  return true;
}

bool CurlHttpClient::CleanupCurlHandleData(CurlHandleDataPtr handle_data) {
  absl::MutexLock l(&running_handles_mutex_);
  for (const auto& handle : running_handles_) {
    if (handle->easy_handle == handle_data->easy_handle) {
      running_handles_.erase(handle);
      handle_data.release();
      return true;
    }
  }
  return false;
}

}  // namespace plusfish
