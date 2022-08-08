#include "ons/ONSFactory.h"
#include "rocketmq/Logger.h"
#include <chrono>
#include <iostream>

using namespace std;
using namespace ons;

int main(int argc, char* argv[]) {
  rocketmq::Logger& logger = rocketmq::getLogger();
  logger.setLevel(rocketmq::Level::Debug);
  logger.init();

  std::cout << "=======Before sending messages=======" << std::endl;
  ONSFactoryProperty factoryInfo;

  Producer* producer = nullptr;
  producer = ONSFactory::getInstance()->createProducer(factoryInfo);
  producer->start();
  Message message("cpp_sdk_standard", "Your Tag", "Your Key", "This message body.");

  // Set a timepoint in the future, after which this message will be available.
  message.setStartDeliverTime(std::chrono::system_clock::now() + std::chrono::seconds(10));

  try {
    SendResultONS sendResult = producer->send(message);
    std::cout << "Message ID: " << sendResult.getMessageId() << std::endl;
  } catch (ONSClientException& e) {
    std::cout << "ErrorCode: " << e.what() << std::endl;
  }

  // Keep main thread running until process finished.
  producer->shutdown();
  std::cout << "=======After sending messages=======" << std::endl;
  return 0;
}