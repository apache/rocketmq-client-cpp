#include "rocketmq/DefaultMQPushConsumer.h"
#include "gtest/gtest.h"

#include <chrono>
#include <thread>

using namespace rocketmq;

class SampleMQMessageListener : public StandardMessageListener {
public:
  ConsumeStatus consumeMessage(const std::vector<MQMessageExt>& msgs) override {
    for (const MQMessageExt& msg : msgs) {
      std::cout << "Topic=" << msg.getTopic() << ", MsgId=" << msg.getMsgId() << ", Body=" << msg.getBody()
                << std::endl;
    }
    return ConsumeStatus::CONSUME_SUCCESS;
  }
};

class DefaultMQPushConsumerTest : public testing::Test {
public:
  DefaultMQPushConsumerTest() : push_consumer_("CID_sample"), listener_(new SampleMQMessageListener) {}

  void SetUp() override {
    push_consumer_.setGroupName("CID_sample");
    push_consumer_.setInstanceName("CID_sample_member_0");
    push_consumer_.subscribe("TopicTest", "*");
    push_consumer_.setNamesrvAddr("localhost:9886");
    push_consumer_.registerMessageListener(listener_);
  }

  void TearDown() override { push_consumer_.shutdown(); }

  ~DefaultMQPushConsumerTest() override { delete listener_; }

protected:
  DefaultMQPushConsumer push_consumer_;
  MessageListener* listener_;
};

TEST_F(DefaultMQPushConsumerTest, testStartupAndShutdown) {
  push_consumer_.setConsumeThreadCount(1);
  push_consumer_.start();
  std::this_thread::sleep_for(std::chrono::minutes(10));
}
