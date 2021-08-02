#include "rocketmq/DefaultMQPushConsumer.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <iostream>

using namespace rocketmq;

class CounterMessageListener : public StandardMessageListener {
public:
  explicit CounterMessageListener(std::atomic_long& counter) : counter_(counter) {}

  ConsumeMessageResult consumeMessage(const std::vector<MQMessageExt>& msgs) override {
    counter_.fetch_add(msgs.size());
    return ConsumeMessageResult::SUCCESS;
  }

private:
  std::atomic_long& counter_;
};

int main(int argc, char* argv[]) {

  Logger& logger = getLogger();
  logger.setLevel(Level::Debug);
  logger.init();

  std::atomic_long counter(0);

  DefaultMQPushConsumer push_consumer("CID_sample");
  MessageListener* listener = new CounterMessageListener(counter);

  push_consumer.setGroupName("CID_sample");
  push_consumer.setInstanceName("CID_sample_member_0");
  push_consumer.subscribe("TopicTest", "*");
  push_consumer.setNamesrvAddr("11.167.164.105:9876");
  push_consumer.registerMessageListener(listener);
  push_consumer.start();

  std::atomic_bool stopped(false);
  std::thread report_thread([&counter, &stopped]() {
    while (!stopped) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      long qps;
      while (true) {
        qps = counter.load(std::memory_order_relaxed);
        if (counter.compare_exchange_weak(qps, 0, std::memory_order_relaxed)) {
          break;
        }
      }
      std::cout << "QPS: " << qps << std::endl;
    }
  });

  std::this_thread::sleep_for(std::chrono::minutes(30));
  stopped.store(true);

  if (report_thread.joinable()) {
    report_thread.join();
  }

  push_consumer.shutdown();
  return EXIT_SUCCESS;
}
