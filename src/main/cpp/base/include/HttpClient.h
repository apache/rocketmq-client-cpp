#pragma once

#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "rocketmq/RocketMQ.h"
#include <curl/curl.h>
#include <string>
#include <thread>

ROCKETMQ_NAMESPACE_BEGIN

class HttpClient {
public:
  HttpClient();

  virtual ~HttpClient();

  bool get(const std::string& query, std::string& response, absl::Duration timeout);

  explicit operator bool() const { return curl_ != nullptr; }

  static std::size_t writeCallback(void* data, std::size_t size, std::size_t nmemb, void* param);

private:
  CURL* curl_;
  absl::Mutex mtx_;
};

ROCKETMQ_NAMESPACE_END