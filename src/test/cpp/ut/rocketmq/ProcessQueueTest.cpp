#include "Assignment.h"
#include "ClientManagerMock.h"
#include "ConsumeMessageType.h"
#include "InvocationContext.h"
#include "MessageAccessor.h"
#include "ProcessQueueImpl.h"
#include "PushConsumerMock.h"
#include "ReceiveMessageCallbackMock.h"
#include "ReceiveMessageResult.h"
#include "RpcClientMock.h"
#include "rocketmq/CredentialsProvider.h"
#include "rocketmq/MQMessageExt.h"
#include "gtest/gtest.h"
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

ROCKETMQ_NAMESPACE_BEGIN

class ProcessQueueTest : public testing::Test {
public:
  void SetUp() override {
    rpc_client_ = std::make_shared<testing::NiceMock<RpcClientMock>>();
    message_queue_.serviceAddress(service_address_);
    message_queue_.setTopic(topic_);
    message_queue_.setBrokerName(broker_name_);
    message_queue_.setQueueId(queue_id_);
    client_manager_ = std::make_shared<testing::NiceMock<ClientManagerMock>>();
    credentials_provider_ = std::make_shared<StaticCredentialsProvider>(access_key_, access_secret_);
    consumer_ = std::make_shared<testing::NiceMock<PushConsumerMock>>();
    auto consumer = std::dynamic_pointer_cast<PushConsumer>(consumer_);
    process_queue_ = std::make_shared<ProcessQueueImpl>(message_queue_, filter_expression_, consumer, client_manager_);
    receive_message_callback_ = std::make_shared<testing::NiceMock<ReceiveMessageCallbackMock>>();
    process_queue_->callback(receive_message_callback_);
  }

  void TearDown() override {}

protected:
  std::string tenant_id_{"tenant-0"};
  std::string access_key_{"ak"};
  std::string access_secret_{"secret"};
  std::shared_ptr<CredentialsProvider> credentials_provider_;
  std::string group_name_{"TestGroup"};
  std::string client_id_{"Client-0"};
  std::string broker_name_{"broker-a"};
  std::string region_{"cn-hangzhou"};
  std::string service_name_{"MQ"};
  int queue_id_{0};
  std::string topic_{"TestTopic"};
  std::string service_address_{"ipv4:10.0.0.1:10911"};
  std::string tag_{"TagA"};
  FilterExpression filter_expression_{tag_};
  MQMessageQueue message_queue_;
  std::shared_ptr<testing::NiceMock<RpcClientMock>> rpc_client_;
  std::shared_ptr<testing::NiceMock<ClientManagerMock>> client_manager_;
  std::shared_ptr<testing::NiceMock<PushConsumerMock>> consumer_;
  std::shared_ptr<ProcessQueueImpl> process_queue_;
  std::shared_ptr<testing::NiceMock<ReceiveMessageCallbackMock>> receive_message_callback_;
  std::string resource_namespace_{"mq://test"};
  std::string message_body_{"Sample body"};

  uint32_t threshold_quantity_{32};
  uint64_t threshold_memory_{4096};
  uint32_t consume_batch_size_{8};
};

TEST_F(ProcessQueueTest, testBind) {
  EXPECT_TRUE(process_queue_->bindFifoConsumeTask());
  EXPECT_FALSE(process_queue_->bindFifoConsumeTask());
  EXPECT_TRUE(process_queue_->unbindFifoConsumeTask());
  EXPECT_FALSE(process_queue_->unbindFifoConsumeTask());
  EXPECT_TRUE(process_queue_->bindFifoConsumeTask());
}

TEST_F(ProcessQueueTest, testExpired) {
  EXPECT_FALSE(process_queue_->expired());
  process_queue_->idle_since_ -= MixAll::PROCESS_QUEUE_EXPIRATION_THRESHOLD_;
  EXPECT_TRUE(process_queue_->expired());
}

TEST_F(ProcessQueueTest, testShouldThrottle) {
  EXPECT_CALL(*consumer_, maxCachedMessageQuantity)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(threshold_quantity_));
  EXPECT_CALL(*consumer_, maxCachedMessageMemory)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(threshold_memory_));
  EXPECT_FALSE(process_queue_->shouldThrottle());
}

TEST_F(ProcessQueueTest, testShouldThrottle_ByQuantity) {
  std::vector<MQMessageExt> messages;
  for (uint32_t i = 0; i < threshold_quantity_; i++) {
    MQMessageExt message;
    message.setTopic(topic_);
    message.setTags(tag_);
    MessageAccessor::setQueueId(message, 0);
    MessageAccessor::setQueueOffset(message, i);
    message.setBody(std::to_string(i));
    messages.emplace_back(message);
  }

  process_queue_->cacheMessages(messages);

  EXPECT_CALL(*consumer_, maxCachedMessageQuantity)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(threshold_quantity_));
  EXPECT_CALL(*consumer_, maxCachedMessageMemory)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(threshold_memory_));
  EXPECT_TRUE(process_queue_->shouldThrottle());
}

TEST_F(ProcessQueueTest, testShouldThrottle_ByMemory) {
  std::vector<MQMessageExt> messages;
  size_t body_length = 1024 * 4;
  for (uint32_t i = 0; i < threshold_quantity_ / 2; i++) {
    MQMessageExt message;
    message.setTopic(topic_);
    message.setTags(tag_);
    MessageAccessor::setQueueId(message, 0);
    MessageAccessor::setQueueOffset(message, i);
    message.setBody(std::string(body_length, 'c'));
    messages.emplace_back(message);
  }

  process_queue_->cacheMessages(messages);

  EXPECT_CALL(*consumer_, maxCachedMessageQuantity)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(threshold_quantity_));
  EXPECT_CALL(*consumer_, maxCachedMessageMemory)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(threshold_memory_));
  EXPECT_TRUE(process_queue_->shouldThrottle());
}

TEST_F(ProcessQueueTest, testHasPendingMessages) { EXPECT_FALSE(process_queue_->hasPendingMessages()); }

TEST_F(ProcessQueueTest, testHasPendingMessages2) {
  std::vector<MQMessageExt> messages;
  size_t body_length = 1024;
  for (size_t i = 0; i < threshold_quantity_; i++) {
    MQMessageExt message;
    message.setTopic(topic_);
    message.setTags(tag_);
    MessageAccessor::setQueueId(message, 0);
    MessageAccessor::setQueueOffset(message, i);
    message.setBody(std::string(body_length, 'c'));
    messages.emplace_back(message);
  }
  process_queue_->cacheMessages(messages);
  EXPECT_TRUE(process_queue_->hasPendingMessages());
}

TEST_F(ProcessQueueTest, testTake) {
  std::vector<MQMessageExt> messages;
  EXPECT_FALSE(process_queue_->take(consume_batch_size_, messages));
  EXPECT_TRUE(messages.empty());
}

TEST_F(ProcessQueueTest, testTake2) {

  {
    std::vector<MQMessageExt> messages;
    size_t body_length = 1024;
    for (size_t i = 0; i < threshold_quantity_; i++) {
      MQMessageExt message;
      message.setTopic(topic_);
      message.setTags(tag_);
      MessageAccessor::setQueueId(message, 0);
      MessageAccessor::setQueueOffset(message, i);
      message.setBody(std::string(body_length, 'c'));
      messages.emplace_back(message);
    }
    process_queue_->cacheMessages(messages);
    EXPECT_EQ(threshold_quantity_, process_queue_->cachedMessagesSize());
  }

  std::vector<MQMessageExt> msgs;
  EXPECT_TRUE(process_queue_->take(consume_batch_size_, msgs));
  EXPECT_FALSE(msgs.empty());
  EXPECT_EQ(tag_, msgs.begin()->getTags());
  EXPECT_EQ(topic_, msgs.begin()->getTopic());
  EXPECT_EQ(threshold_quantity_ - consume_batch_size_, process_queue_->cachedMessagesSize());
}

TEST_F(ProcessQueueTest, testRelease) {
  EXPECT_CALL(*consumer_, messageModel)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(MessageModel::BROADCASTING));

  int64_t offset;
  EXPECT_FALSE(process_queue_->committedOffset(offset));

  size_t body_length = 1024;
  {
    std::vector<MQMessageExt> messages;
    for (size_t i = 0; i < threshold_quantity_; i++) {
      MQMessageExt message;
      message.setTopic(topic_);
      message.setTags(tag_);
      MessageAccessor::setQueueId(message, 0);
      MessageAccessor::setQueueOffset(message, i);
      message.setBody(std::string(body_length, 'c'));
      messages.emplace_back(message);
    }
    process_queue_->cacheMessages(messages);
    EXPECT_EQ(threshold_quantity_, process_queue_->cachedMessagesSize());
  }

  std::vector<MQMessageExt> msgs;
  process_queue_->take(1, msgs);

  EXPECT_TRUE(process_queue_->committedOffset(offset));
  EXPECT_EQ(0, offset);

  process_queue_->release(body_length, 0);

  EXPECT_TRUE(process_queue_->committedOffset(offset));
  EXPECT_EQ(1, offset);
}

TEST_F(ProcessQueueTest, testOffset) {
  EXPECT_CALL(*consumer_, messageModel)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(MessageModel::BROADCASTING));

  int64_t offset;
  EXPECT_FALSE(process_queue_->committedOffset(offset));

  size_t body_length = 1024;
  {
    std::vector<MQMessageExt> messages;
    for (size_t i = 0; i < threshold_quantity_; i++) {
      MQMessageExt message;
      message.setTopic(topic_);
      message.setTags(tag_);
      MessageAccessor::setQueueId(message, 0);
      MessageAccessor::setQueueOffset(message, i);
      message.setBody(std::string(body_length, 'c'));
      messages.emplace_back(message);
    }
    process_queue_->cacheMessages(messages);
    EXPECT_EQ(threshold_quantity_, process_queue_->cachedMessagesSize());
  }

  std::vector<MQMessageExt> msgs;
  process_queue_->take(threshold_quantity_, msgs);

  EXPECT_TRUE(process_queue_->committedOffset(offset));
  EXPECT_EQ(0, offset);

  for (size_t i = 0; i < threshold_quantity_; i++) {
    process_queue_->release(body_length, i);
  }

  EXPECT_TRUE(process_queue_->committedOffset(offset));
  EXPECT_EQ(threshold_quantity_, offset);
}

TEST_F(ProcessQueueTest, testReceiveMessage_POP) {
  EXPECT_CALL(*consumer_, tenantId).WillRepeatedly(testing::ReturnRef(tenant_id_));
  EXPECT_CALL(*consumer_, resourceNamespace).WillRepeatedly(testing::ReturnRef(resource_namespace_));
  EXPECT_CALL(*consumer_, credentialsProvider).WillRepeatedly(testing::Return(credentials_provider_));
  EXPECT_CALL(*consumer_, region).WillRepeatedly(testing::ReturnRef(region_));
  EXPECT_CALL(*consumer_, serviceName).WillRepeatedly(testing::ReturnRef(service_name_));
  EXPECT_CALL(*consumer_, clientId).WillRepeatedly(testing::Return(client_id_));
  EXPECT_CALL(*consumer_, getGroupName).WillRepeatedly(testing::ReturnRef(group_name_));
  EXPECT_CALL(*consumer_, getLongPollingTimeout).WillRepeatedly(testing::Return(absl::Seconds(3)));
  EXPECT_CALL(*consumer_, receiveMessageAction).WillRepeatedly(testing::Return(ReceiveMessageAction::POLLING));

  auto optional = absl::make_optional(filter_expression_);

  EXPECT_CALL(*consumer_, getFilterExpression).WillRepeatedly(testing::Return(optional));
  EXPECT_CALL(*consumer_, receiveBatchSize).WillRepeatedly(testing::Return(threshold_quantity_));

  auto receive_message_mock = [this](const std::string& target, const Metadata& metadata,
                                     const ReceiveMessageRequest& request, std::chrono::milliseconds timeout,
                                     const std::shared_ptr<ReceiveMessageCallback>& cb) {
    ReceiveMessageResult receive_message_result;
    receive_message_result.status_ = ReceiveMessageStatus::OK;

    for (size_t i = 0; i < threshold_quantity_; i++) {
      MQMessageExt message;
      message.setTopic(topic_);
      message.setTags(tag_);
      message.setBody(message_body_);
      MessageAccessor::setQueueId(message, queue_id_);
      MessageAccessor::setQueueOffset(message, i);
      receive_message_result.messages_.emplace_back(message);
    }
    cb->onSuccess(receive_message_result);
  };

  EXPECT_CALL(*client_manager_, receiveMessage)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(receive_message_mock));
  process_queue_->receiveMessage();
}

TEST_F(ProcessQueueTest, testReceiveMessage_Pull) {
  EXPECT_CALL(*consumer_, tenantId).WillRepeatedly(testing::ReturnRef(tenant_id_));
  EXPECT_CALL(*consumer_, resourceNamespace).WillRepeatedly(testing::ReturnRef(resource_namespace_));
  EXPECT_CALL(*consumer_, credentialsProvider).WillRepeatedly(testing::Return(credentials_provider_));
  EXPECT_CALL(*consumer_, region).WillRepeatedly(testing::ReturnRef(region_));
  EXPECT_CALL(*consumer_, serviceName).WillRepeatedly(testing::ReturnRef(service_name_));
  EXPECT_CALL(*consumer_, clientId).WillRepeatedly(testing::Return(client_id_));
  EXPECT_CALL(*consumer_, getGroupName).WillRepeatedly(testing::ReturnRef(group_name_));
  EXPECT_CALL(*consumer_, getLongPollingTimeout).WillRepeatedly(testing::Return(absl::Seconds(3)));
  EXPECT_CALL(*consumer_, receiveMessageAction).WillRepeatedly(testing::Return(ReceiveMessageAction::PULL));

  auto optional = absl::make_optional(filter_expression_);

  EXPECT_CALL(*consumer_, getFilterExpression).WillRepeatedly(testing::Return(optional));
  EXPECT_CALL(*consumer_, receiveBatchSize).WillRepeatedly(testing::Return(threshold_quantity_));

  auto invocation_context = new InvocationContext<PullMessageResponse>();
  auto pull_message_mock = [&](const std::string& target_host, const Metadata& metadata,
                               const PullMessageRequest& request, std::chrono::milliseconds timeout,
                               const std::function<void(const InvocationContext<PullMessageResponse>*)>& cb) {
    cb(invocation_context);
  };

  EXPECT_CALL(*client_manager_, pullMessage)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(pull_message_mock));
  EXPECT_CALL(*client_manager_, processPullResult);
  process_queue_->receiveMessage();
}

ROCKETMQ_NAMESPACE_END