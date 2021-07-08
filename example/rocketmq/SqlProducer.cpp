#include "rocketmq/DefaultMQProducer.h"
#include <iostream>

using namespace rocketmq;

int main(int argc, char* argv[]) {

  Logger& logger = getLogger();
  logger.setLevel(Level::Debug);
  logger.init();

  DefaultMQProducer producer("PID_sample");
  producer.setNamesrvAddr("11.167.164.105:9876");

  MQMessage message;
  message.setTopic("TestTopic");
  try {
    producer.start();
    for (int i = 0; i < 8; ++i) {
      std::string body = std::to_string(i);
      message.setBody(body);
      message.setProperty("a", std::to_string(i % 5));
      switch (i % 3) {
      case 0:
        message.setTags("TagA");
        break;
      case 1:
        message.setTags("TagB");
        break;
      case 2:
        message.setTags("TagC");
        break;
      }
      SendResult sendResult = producer.send(message);
      std::cout << "Message sent with msgId=" << sendResult.getMsgId()
                << ", Queue=" << sendResult.getMessageQueue().simpleName() << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  } catch (...) {
    std::cerr << "Ah...No!!!" << std::endl;
  }
  producer.shutdown();
  return EXIT_SUCCESS;
}