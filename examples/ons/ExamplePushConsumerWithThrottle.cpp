#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

#include "ons/MessageModel.h"
#include "ons/ONSFactory.h"

#include "rocketmq/Logger.h"

using namespace std;
using namespace ons;

std::mutex console_mtx;

class ExampleMessageListener : public MessageListener {
public:
  Action consume(const Message& message, ConsumeContext& context) noexcept override {
    std::lock_guard<std::mutex> lk(console_mtx);
    auto latency = std::chrono::system_clock::now() - message.getStoreTimestamp();
    auto latency2 = std::chrono::system_clock::now() - message.getBornTimestamp();
    std::cout << "Received a message. Topic: " << message.getTopic() << ", MsgId: " << message.getMsgID()
              << ", Body-size: " << message.getBody().size() << ", Tag: " << message.getTag()
              << ", Current - Store-Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(latency).count()
              << "ms, Current - Born-Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(latency2).count()
              << "ms" << std::endl;
    return Action::CommitMessage;
  }
};

int main(int argc, char* argv[]) {
  auto& logger = rocketmq::getLogger();
  logger.setLevel(rocketmq::Level::Debug);
  logger.init();

  const char* topic = "cpp_sdk_standard";
  const char* tag = "*";

  std::cout << "=======Before consuming messages=======" << std::endl;
  ONSFactoryProperty factory_property;
  factory_property.setFactoryProperty(ons::ONSFactoryProperty::GroupId, "GID_cpp_sdk_standard");

  // Client-side throttling
  factory_property.throttle(topic, 16);

  PushConsumer* consumer = ONSFactory::getInstance()->createPushConsumer(factory_property);

  // register your own listener here to handle the messages received.
  auto* messageListener = new ExampleMessageListener();
  consumer->subscribe(topic, tag);
  consumer->registerMessageListener(messageListener);

  // Start this consumer
  consumer->start();

  // Keep main thread running until process finished.
  std::this_thread::sleep_for(std::chrono::minutes(15));

  consumer->shutdown();
  std::cout << "=======After consuming messages======" << std::endl;
  return 0;
}