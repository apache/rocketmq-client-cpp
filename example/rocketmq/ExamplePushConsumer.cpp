#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

#include "rocketmq/Logger.h"

#include "spdlog/spdlog.h"

#include "rocketmq/DefaultMQPushConsumer.h"

using namespace rocketmq;

class SampleMQMessageListener : public StandardMessageListener {
public:
  ConsumeMessageResult consumeMessage(const std::vector<MQMessageExt>& msgs) override {
    for (const MQMessageExt& msg : msgs) {
      SPDLOG_INFO("Consume message[Topic={}, MessageId={}] OK", msg.getTopic(), msg.getMsgId());
      std::cout << "Consume Message[MsgId=" << msg.getMsgId() << "] OK" << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return ConsumeMessageResult::SUCCESS;
  }
};

int main(int argc, char* argv[]) {

  Logger& logger = getLogger();
  logger.setLevel(Level::Debug);
  logger.init();

  const char* group_id = "GID_cpp_sdk_standard";
  const char* topic = "cpp_sdk_standard";
  const char* resource_namespace = "MQ_INST_1080056302921134_BXuIbML7";

  DefaultMQPushConsumer push_consumer(group_id);
  push_consumer.setResourceNamespace(resource_namespace);
  push_consumer.setCredentialsProvider(std::make_shared<ConfigFileCredentialsProvider>());
  push_consumer.setNamesrvAddr("47.98.116.189:80");
  MessageListener* listener = new SampleMQMessageListener;
  push_consumer.setInstanceName("instance_0");
  // push_consumer.setGroupName(group_id);
  push_consumer.subscribe(topic, "*");
  push_consumer.registerMessageListener(listener);
  push_consumer.setConsumeThreadCount(4);
  push_consumer.start();

  std::this_thread::sleep_for(std::chrono::seconds(300));

  push_consumer.shutdown();
  return EXIT_SUCCESS;
}
