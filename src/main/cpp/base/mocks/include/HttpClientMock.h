#pragma once

#include "gmock/gmock.h"
#include "HttpClient.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class HttpClientMock : public HttpClient {
public:
  ~HttpClientMock() override = default;
  
  MOCK_METHOD(void, start, (), (override));
  MOCK_METHOD(void, shutdown, (), (override));
  MOCK_METHOD(void, get, (HttpProtocol, const std::string&, std::uint16_t, const std::string&,
      const std::function<void(int, const absl::flat_hash_map<std::string, std::string>&, const std::string&)>&), (override));
};

ROCKETMQ_NAMESPACE_END