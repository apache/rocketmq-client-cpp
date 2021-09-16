#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

enum class HttpProtocol : int8_t {
  HTTP = 1,
  HTTPS = 2,
};

enum class HttpStatus : int {
  OK = 200,
  INTERNAL = 500,
};

class HttpClient {
public:
  virtual ~HttpClient() = default;

  virtual void start() = 0;

  virtual void shutdown() = 0;

  virtual void
  get(HttpProtocol protocol, const std::string& host, std::uint16_t port, const std::string& path,
      const std::function<void(int, const std::multimap<std::string, std::string>&, const std::string&)>& cb) = 0;
};

ROCKETMQ_NAMESPACE_END