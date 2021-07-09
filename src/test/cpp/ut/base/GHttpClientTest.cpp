#include "GHttpClient.h"
#include "LoggerImpl.h"
#include "rocketmq/RocketMQ.h"
#include "gtest/gtest.h"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
ROCKETMQ_NAMESPACE_BEGIN

class GHttpClientTest : public testing::Test {
public:
  void SetUp() override {
    SPDLOG_DEBUG("GHttpClient::SetUp() starts");
    grpc_init();
    SPDLOG_DEBUG("GHttpClient::SetUp() completed");
  }

  void TearDown() override {
    SPDLOG_DEBUG("GHttpClientTest::TearDown() starts");
    grpc_shutdown();
    SPDLOG_DEBUG("GHttpClientTest::TearDown() completed");
  }
};

TEST_F(GHttpClientTest, testCtor) {
  spdlog::set_level(spdlog::level::debug);
  GHttpClient http_client;
  absl::Mutex mtx;
  absl::CondVar cv;
  bool completed = false;
  auto callback = [&](int status, const absl::flat_hash_map<std::string, std::string>& metadata,
                      const std::string& body) {
    absl::MutexLock lk(&mtx);
    completed = true;
    SPDLOG_DEBUG("HTTP status-code: {}, response body: {}", status, body);
    cv.SignalAll();
  };

  std::string host("grpc.io");
  http_client.start();
  http_client.get(HttpProtocol::HTTP, host, 8080, "/", callback);
  if (!completed) {
    absl::MutexLock lk(&mtx);
    cv.WaitWithTimeout(&mtx, absl::Seconds(5));
  }
  ASSERT_TRUE(completed);
  http_client.shutdown();
}

ROCKETMQ_NAMESPACE_END