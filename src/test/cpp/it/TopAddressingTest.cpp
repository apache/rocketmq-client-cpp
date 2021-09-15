#include "TopAddressing.h"
#include "RateLimiter.h"
#include "grpc/grpc.h"
#include "spdlog/spdlog.h"
#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <thread>

ROCKETMQ_NAMESPACE_BEGIN

class TopAddressingTest : public testing::Test {
public:
  void SetUp() override {
    grpc_init();
    spdlog::set_level(spdlog::level::debug);
  }

  void TearDown() override { grpc_shutdown(); }

  void SetEnv(const char* key, const char* value) {
    int overwrite = 1;
    #ifdef _WIN32
    std::string env;
    env.append(key);
    env.push_back('=');
    env.append(value);
    _putenv(env.c_str());
    #else
    setenv(key, value, overwrite);
    #endif
  }
};

TEST_F(TopAddressingTest, testFetchNameServerAddresses) {
  std::vector<std::string> list;
  absl::Mutex mtx;
  absl::CondVar cv;
  bool completed = false;

  bool success = false;
  auto callback = [&](bool ok, const std::vector<std::string>& name_server_list) {
    success = ok;
    list.insert(list.end(), name_server_list.begin(), name_server_list.end());
    {
      absl::MutexLock lk(&mtx);
      completed = true;
      cv.SignalAll();
    }
  };
  TopAddressing top_addressing;
  top_addressing.fetchNameServerAddresses(callback);

  while (!completed) {
    absl::MutexLock lk(&mtx);
    if (!completed) {
      cv.WaitWithTimeout(&mtx, absl::Seconds(3));
    }
  }

  ASSERT_TRUE(success);
  EXPECT_FALSE(list.empty());
}

TEST_F(TopAddressingTest, testFetchNameServerAddresses_env) {
  SetEnv(HostInfo::ENV_LABEL_UNIT, "CENTER_UNIT.center");
  SetEnv(HostInfo::ENV_LABEL_STAGE, "DAILY");
  std::vector<std::string> list;

  absl::Mutex mtx;
  absl::CondVar cv;
  bool completed = false;

  bool success = false;
  auto callback = [&](bool ok, const std::vector<std::string>& name_server_list) {
    success = ok;
    list.insert(list.end(), name_server_list.begin(), name_server_list.end());
    {
      absl::MutexLock lk(&mtx);
      completed = true;
      cv.SignalAll();
    }
  };
  TopAddressing top_addressing;
  top_addressing.fetchNameServerAddresses(callback);

  while (!completed) {
    absl::MutexLock lk(&mtx);
    if (!completed) {
      cv.WaitWithTimeout(&mtx, absl::Seconds(3));
    }
  }

  ASSERT_TRUE(success);
  EXPECT_FALSE(list.empty());
}

TEST_F(TopAddressingTest, testPerf) {
  TopAddressing top_addressing;
  RateLimiter<10> rate_limiter(100);

  std::atomic_bool stopped(false);
  std::atomic_long qps(0);
  auto callback = [&](bool ok, const std::vector<std::string>& name_sever_list) {
    if (ok) {
      qps++;
    } else {
      SPDLOG_WARN("Yuck, Bad HTTP response");
    }
  };

  auto benchmark = [&]() {
    while (!stopped) {
      // rate_limiter.acquire();
      SPDLOG_DEBUG("Submit a fetch request");
      top_addressing.fetchNameServerAddresses(callback);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  };

  auto stats = [&]() {
    while (!stopped) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      long value = qps.load(std::memory_order_relaxed);
      qps.fetch_sub(value, std::memory_order_relaxed);
      SPDLOG_INFO("QPS: {}", value);
    }
  };

  std::thread benchmark_thread(benchmark);
  std::thread stats_thread(stats);

  std::this_thread::sleep_for(std::chrono::seconds(5));
  stopped.store(true);

  if (stats_thread.joinable()) {
    stats_thread.join();
  }

  if (benchmark_thread.joinable()) {
    benchmark_thread.join();
  }
}

ROCKETMQ_NAMESPACE_END