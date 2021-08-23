#include "rocketmq/DefaultMQProducer.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <random>

using namespace rocketmq;

int getAndReset(std::atomic_int& counter) {
  int current;
  while (true) {
    current = counter.load(std::memory_order_relaxed);
    if (counter.compare_exchange_weak(current, 0, std::memory_order_relaxed)) {
      break;
    }
  }
  return current;
}

const std::string& alphaNumeric() {
  static std::string alpha_numeric("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
  return alpha_numeric;
}

std::string randomString(std::string::size_type len) {
  std::string result;
  result.reserve(len);
  std::random_device rd;
  std::mt19937 generator(rd());
  std::string source(alphaNumeric());
  std::string::size_type generated = 0;
  while (generated < len) {
    std::shuffle(source.begin(), source.end(), generator);
    std::string::size_type delta = std::min({len - generated, source.length()});
    result.append(source.substr(0, delta));
    generated += delta;
  }
  return result;
}

template <int PARTITION> class RateLimiter {
public:
  explicit RateLimiter(int permit) : permits_{0}, interval_(1000 / PARTITION), stopped_(false) {
    int avg = permit / PARTITION;
    for (auto& i : partition_) {
      i = avg;
    }

    int r = permit % PARTITION;

    if (r) {
      int step = PARTITION / r;
      for (int i = 0; i < r; ++i) {
        partition_[i * step]++;
      }
    }

    auto lambda_tick = [this]() {
      while (!stopped_) {
        tick();
      }
    };

    thread_tick_ = std::thread(lambda_tick);
  }

  ~RateLimiter() {
    stopped_.store(true, std::memory_order_relaxed);
    if (thread_tick_.joinable()) {
      thread_tick_.join();
    }
  }

  std::array<int, PARTITION>& partition() { return permits_; }

  int slot() {
    auto current = std::chrono::steady_clock::now();
    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(current.time_since_epoch()).count();
    return ms / interval_ % PARTITION;
  }

  void acquire() {
    int idx = slot();
    {
      std::unique_lock<std::mutex> lk(mtx_);
      if (permits_[idx] > 0) {
        --permits_[idx];
        return;
      }

      // Reuse quota of the past half of the cycle
      for (int j = 1; j <= PARTITION / 2; ++j) {
        int index = idx - j;
        if (index < 0) {
          index += PARTITION;
        }
        if (permits_[index] > 0) {
          --permits_[index];
          return;
        }
      }

      cv_.wait(lk, [this]() {
        int idx = slot();
        return permits_[idx] > 0;
      });
      idx = slot();
      --permits_[idx];
    }
  }

  void tick() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 / PARTITION));
    int idx = slot();
    {
      std::unique_lock<std::mutex> lk(mtx_);
      permits_[idx] = partition_[idx];
      cv_.notify_all();
    }
  }

private:
  std::array<int, PARTITION> partition_;
  std::array<int, PARTITION> permits_;
  int interval_;
  std::mutex mtx_;
  std::condition_variable cv_;

  std::atomic_bool stopped_;
  std::thread thread_tick_;
};

class SampleSendCallback : public rocketmq::SendCallback {
public:
  SampleSendCallback(std::atomic_int& counter, std::atomic_int& error) : counter_(counter), error_(error) {}

  void onSuccess(SendResult& send_result) override { counter_.fetch_add(1, std::memory_order_relaxed); }

  void onException(const MQException& e) override { error_.fetch_add(1, std::memory_order_relaxed); }

private:
  std::atomic_int& counter_;
  std::atomic_int& error_;
};

int main(int argc, char* argv[]) {
  std::string::size_type body_size = 1024;
  int duration = 300;
  int qps = 1000;

  if (argc > 1) {
    body_size = std::stoi(argv[1]);
  }

  if (argc > 2) {
    duration = std::stoi(argv[2]);
  }

  if (argc > 3) {
    qps = std::stoi(argv[3]);
  }

  Logger& logger = getLogger();
  logger.setLevel(Level::Debug);
  logger.init();

  DefaultMQProducer producer("TestGroup");
  producer.setNamesrvAddr("11.165.223.199:9876");
  producer.compressBodyThreshold(256);
  const char* arn = "MQ_INST_1973281269661160_BXmPlOA6";
  producer.setCredentialsProvider(std::make_shared<ConfigFileCredentialsProvider>());
  producer.arn(arn);
  producer.setRegion("cn-hangzhou");
  MQMessage message;
  message.setTopic("yc001");
  message.setTags("TagA");
  message.setKey("Yuck! Why-plural?");
  message.setBody(randomString(body_size));

  std::cout << "Message Body: " << message.getBody() << std::endl;

  std::atomic_bool stopped(false);

  auto lambda = [&stopped, duration]() {
    std::cout << "Benchmark will stop in " << duration << " seconds!" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(duration));
    stopped.store(true);
  };

  std::thread t(lambda);

  std::atomic_int success(0);
  std::atomic_int error(0);

  auto stats_lambda = [&stopped, &success, &error]() {
    while (!stopped) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::cout << "Success:" << getAndReset(success) << ", Error: " << getAndReset(error) << std::endl;
    }
  };

  std::thread stats_thread(stats_lambda);

  RateLimiter<10> rate_limiter(qps);
  SampleSendCallback send_callback(success, error);

  try {
    producer.start();
    while (!stopped) {
      rate_limiter.acquire();
      producer.send(message, &send_callback, true);
    }
  } catch (...) {
    std::cerr << "Ah...No!!!" << std::endl;
  }

  if (stats_thread.joinable()) {
    stats_thread.join();
  }

  if (t.joinable()) {
    t.join();
  }
  producer.shutdown();
  return EXIT_SUCCESS;
}