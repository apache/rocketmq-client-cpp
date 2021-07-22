#pragma once
#include "absl/container/flat_hash_map.h"
#include "rocketmq/RocketMQ.h"
#include <cstdint>
#include <functional>

ROCKETMQ_NAMESPACE_BEGIN

enum class HttpProtocol : int8_t {
  HTTP = 1,
  HTTPS = 2,
};

class HttpClient {
public:
  virtual ~HttpClient() = default;

  virtual void start() = 0;

  virtual void shutdown() = 0;

  virtual void
  get(HttpProtocol protocol, const std::string& host, std::uint16_t port, const std::string& path,
      const std::function<void(int, const absl::flat_hash_map<std::string, std::string>&, const std::string&)>& cb) = 0;
};

ROCKETMQ_NAMESPACE_END