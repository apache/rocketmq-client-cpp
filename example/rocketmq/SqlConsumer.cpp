#include "rocketmq/DefaultMQPushConsumer.h"

#include <chrono>
#include <mutex>
#include <thread>
#include <iostream>

using namespace rocketmq;

class SampleMQMessageListener : public StandardMessageListener {
public:
  ConsumeMessageResult consumeMessage(const std::vector<MQMessageExt>& msgs) override {
    std::lock_guard<std::mutex> lk(console_mtx_);
    for (const MQMessageExt& msg : msgs) {
      std::cout << "Topic=" << msg.getTopic() << ", MsgId=" << msg.getMsgId() << ", Tag=" << msg.getTags()
                << ", a=" << msg.getProperty("a") << ", Body=" << msg.getBody() << std::endl;
    }
    return ConsumeMessageResult::SUCCESS;
  }

private:
  std::mutex console_mtx_;
};

int main(int argc, char* argv[]) {
  Logger& logger = getLogger();
  logger.setLevel(Level::Debug);
  logger.init();

  DefaultMQPushConsumer push_consumer("TestGroup");
  MessageListener* listener = new SampleMQMessageListener;

  push_consumer.setGroupName("TestGroup");
  push_consumer.setInstanceName("CID_sample_member_0");
  std::string sql_filter("(TAGS is not null and TAGS in ('TagA', 'TagB')) and (a is not null and a between 0 and 3)");
  push_consumer.subscribe("TestTopic", sql_filter, ExpressionType::SQL92);
  // push_consumer.setNamesrvAddr("11.167.164.105:9876");
  push_consumer.registerMessageListener(listener);
  push_consumer.start();

  std::this_thread::sleep_for(std::chrono::seconds(30));

  push_consumer.shutdown();
  return EXIT_SUCCESS;
}
