#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "ons/ONSFactory.h"
#include "rocketmq/Logger.h"
#include "spdlog/spdlog.h"

using namespace ons;

class ExampleMessageOrderListener : public ons::MessageOrderListener {
public:
  OrderAction consume(const Message& message, const ConsumeOrderContext& context) noexcept override {
    SPDLOG_INFO("Consume message[MsgId={}] OK", message.getMsgID());
    std::cout << "Consume Message[MsgId=" << message.getMsgID() << "] OK" << std::endl;
    return OrderAction::Success;
  }
};

int main(int argc, char* argv[]) {
  rocketmq::Logger& logger = rocketmq::getLogger();
  logger.setLevel(rocketmq::Level::Debug);
  logger.init();

  std::cout << "=======Before consuming messages=======" << std::endl;
  ONSFactoryProperty factory_property;

  factory_property.setFactoryProperty(ons::ONSFactoryProperty::GroupId, "GID_cpp_sdk_standard");

  OrderConsumer* consumer = ONSFactory::getInstance()->createOrderConsumer(factory_property);
  const char* topic = "cpp_sdk_standard";
  const char* tag = "*";

  consumer->subscribe(topic, tag);

  auto listener = new ExampleMessageOrderListener;
  consumer->registerMessageListener(listener);

  consumer->start();

  std::this_thread::sleep_for(std::chrono::minutes(3));

  consumer->shutdown();

  return EXIT_SUCCESS;
}