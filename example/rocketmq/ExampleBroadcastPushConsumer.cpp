#include "rocketmq/DefaultMQPushConsumer.h"
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include "spdlog/spdlog.h"

#include <chrono>
#include <mutex>
#include <thread>

using namespace rocketmq;

class SampleMQMessageListener : public StandardMessageListener {
public:
  ConsumeMessageResult consumeMessage(const std::vector<MQMessageExt>& msgs) override {
    for (const MQMessageExt& msg : msgs) {
      SPDLOG_INFO("Receive a message. MessageId={}", msg.getMsgId());
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return ConsumeMessageResult::SUCCESS;
  }
};

int main(int argc, char* argv[]) {

  Logger& logger = getLogger();
  logger.setLevel(Level::Debug);
  logger.init();

  const char* cid = "GID_group003";
  const char *topic = "yc001";
  const char* arn = "MQ_INST_1973281269661160_BXmPlOA6";

  DefaultMQPushConsumer push_consumer(cid);
  push_consumer.setMessageModel(MessageModel::BROADCASTING);
  push_consumer.setArn(arn);
  push_consumer.setCredentialsProvider(std::make_shared<ConfigFileCredentialsProvider>());
  push_consumer.setNamesrvAddr("11.165.223.199:9876");
  MessageListener* listener = new SampleMQMessageListener;
  push_consumer.setGroupName(cid);
  push_consumer.setInstanceName("instance_0");
  push_consumer.subscribe(topic, "*");
  push_consumer.registerMessageListener(listener);
  push_consumer.start();

  std::this_thread::sleep_for(std::chrono::minutes(60));

  push_consumer.shutdown();
  return EXIT_SUCCESS;
}
