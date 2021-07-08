#include "HttpClient.h"
#include <curl/curl.h>
#include <spdlog/spdlog.h>

ROCKETMQ_NAMESPACE_BEGIN

HttpClient::HttpClient() {
  curl_ = curl_easy_init();
  if (!curl_) {
    return;
  }
}

HttpClient::~HttpClient() {
  if (curl_) {
    curl_easy_cleanup(curl_);
    curl_ = nullptr;
  }
}

std::size_t HttpClient::writeCallback(void* data, std::size_t size, std::size_t nmemb, void* param) {
  auto response = reinterpret_cast<std::string*>(param);
  response->reserve(size * nmemb);
  response->append(reinterpret_cast<const char*>(data), size * nmemb);
  return size * nmemb;
}

bool HttpClient::get(const std::string& query, std::string& response, absl::Duration timeout) {
  SPDLOG_DEBUG("GET {}", query);
  if (!curl_) {
    SPDLOG_WARN("CURL session is not properly");
    return false;
  }
  CURLcode res;

  // CURL easy-interface is not thread-safe.
  absl::MutexLock lk(&mtx_);

  SPDLOG_DEBUG("Reset CURL session");
  curl_easy_reset(curl_);

  curl_easy_setopt(curl_, CURLOPT_URL, query.c_str());
  SPDLOG_DEBUG("Set CURL url: {}", query);

  curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, &HttpClient::writeCallback);
  curl_easy_setopt(curl_, CURLOPT_WRITEDATA, (void*)&response);

  SPDLOG_DEBUG("Set CURL IO timeout to {}ms", absl::ToInt64Milliseconds(timeout));
  curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, absl::ToInt64Milliseconds(timeout));

  SPDLOG_DEBUG("Set CURL connect timeout to {}ms", absl::ToInt64Milliseconds(timeout));
  curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT_MS, absl::ToInt64Milliseconds(timeout));

  SPDLOG_DEBUG("Prepare to initiate HTTP/GET request");
  res = curl_easy_perform(curl_);
  SPDLOG_DEBUG("CURL completes request/response");

  if (CURLE_OK == res) {
    SPDLOG_DEBUG("Response code: {}, text: {}", res, response);
    return true;
  } else {
    SPDLOG_WARN("Response code: {}, text: {}", res, response);
    return false;
  }
}

ROCKETMQ_NAMESPACE_END