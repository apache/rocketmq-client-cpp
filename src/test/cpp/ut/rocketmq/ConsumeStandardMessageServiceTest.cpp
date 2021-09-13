#include "ConsumeMessageService.h"
#include "MessageListenerMock.h"
#include "ProcessQueueMock.h"
#include "PushConsumerMock.h"
#include "grpc/grpc.h"
#include "rocketmq/CredentialsProvider.h"
#include "rocketmq/MQMessageExt.h"
#include "gtest/gtest.h"
#include <memory>

ROCKETMQ_NAMESPACE_BEGIN

class ConsumeStandardMessageServiceTest : public testing::Test {
public:
  ConsumeStandardMessageServiceTest()
      : credentials_provider_(std::make_shared<StaticCredentialsProvider>("access_key", "access_secret")) {}

  void SetUp() override {
    grpc_init();
    consumer_ = std::make_shared<testing::NiceMock<PushConsumerMock>>();
    ON_CALL(*consumer_, consumeBatchSize).WillByDefault(testing::Return(consume_batch_size_));
    ON_CALL(*consumer_, messageModel).WillByDefault(testing::Return(MessageModel::CLUSTERING));
    std::weak_ptr<PushConsumer> consumer = std::dynamic_pointer_cast<PushConsumer>(consumer_);
    consume_standard_message_service_ =
        std::make_shared<ConsumeStandardMessageService>(consumer, thread_count_, &message_listener_);
    process_queue_ = std::make_shared<testing::NiceMock<ProcessQueueMock>>();
    ON_CALL(*process_queue_, topic).WillByDefault(testing::Return(topic_));
    auto mock_take = [this](uint32_t batch_size, std::vector<MQMessageExt>& messages) {
      MQMessageExt message;
      message.setTopic(topic_);
      message.setTags(tag_);
      message.setBody(body_);
      messages.emplace_back(message);
      return false;
    };
    ON_CALL(*process_queue_, take).WillByDefault(testing::Invoke(mock_take));
    ON_CALL(*process_queue_, getConsumer).WillByDefault(testing::Return(consumer));
    ON_CALL(*consumer_, customExecutor).WillByDefault(testing::ReturnRef(executor_));
    ON_CALL(*consumer_, credentialsProvider).WillByDefault(testing::Return(credentials_provider_));
    ON_CALL(*consumer_, resourceNamespace).WillByDefault(testing::ReturnRef(resource_namespace_));
    ON_CALL(*consumer_, getGroupName).WillByDefault(testing::ReturnRef(group_name_));
  }

  void TearDown() override { grpc_shutdown(); }

protected:
  int thread_count_{2};
  std::string topic_{"TestTopic"};
  std::string tag_{"TagA"};
  std::string body_{"Body Content"};
  std::string resource_namespace_{"mq://test"};
  std::string group_name_{"CID_Test"};
  uint32_t consume_batch_size_;
  std::shared_ptr<testing::NiceMock<PushConsumerMock>> consumer_;
  std::shared_ptr<ConsumeStandardMessageService> consume_standard_message_service_;
  testing::NiceMock<StandardMessageListenerMock> message_listener_;
  std::shared_ptr<testing::NiceMock<ProcessQueueMock>> process_queue_;
  std::shared_ptr<CredentialsProvider> credentials_provider_;
  Executor executor_;
};

TEST_F(ConsumeStandardMessageServiceTest, testStartAndShutdown) {
  consume_standard_message_service_->start();
  consume_standard_message_service_->shutdown();
}

TEST_F(ConsumeStandardMessageServiceTest, testConsume) {
  ASSERT_FALSE(executor_);
  consume_standard_message_service_->start();

  auto callback = [this](const std::function<void(ProcessQueueSharedPtr)>& cb) { cb(process_queue_); };

  ON_CALL(*consumer_, iterateProcessQueue).WillByDefault(testing::Invoke(callback));

  bool completed = false;
  bool success = false;
  absl::Mutex mtx;
  absl::CondVar cv;

  auto listener_cb = [&](const std::vector<MQMessageExt>& messages) {
    absl::MutexLock lk(&mtx);
    completed = true;
    success = !messages.empty();
    cv.SignalAll();
    return ConsumeMessageResult::SUCCESS;
  };

  ON_CALL(message_listener_, consumeMessage).WillByDefault(testing::Invoke(listener_cb));
  EXPECT_CALL(*process_queue_, release).Times(testing::AtLeast(1));
  EXPECT_CALL(*consumer_, ack).Times(testing::AtLeast(1));

  absl::MutexLock lk(&mtx);
  cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));

  consume_standard_message_service_->shutdown();
}

ROCKETMQ_NAMESPACE_END