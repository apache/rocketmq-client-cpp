#include "rocketmq/DefaultMQPushConsumer.h"
#include "ClientManagerFactory.h"
#include "DefaultMQPushConsumerImpl.h"
#include "InvocationContext.h"
#include "MQClientTest.h"
#include "absl/time/time.h"
#include "rocketmq/MQMessageExt.h"
#include "spdlog/spdlog.h"
#include <atomic>

using namespace testing;

ROCKETMQ_NAMESPACE_BEGIN

std::mutex mtx_;
std::condition_variable cv;
int max_consumer_message_size = 50;
std::atomic_int message_count_(max_consumer_message_size);
std::atomic_int message_index(0);

class DefaultMQPushConsumerUnitTest : public MQClientTest {
public:
  DefaultMQPushConsumerUnitTest() : MQClientTest() {}

  void SetUp() override {
    MQClientTest::SetUp();
    rpc_client_for_broker_ = std::make_shared<NiceMock<RpcClientMock>>();

    testing::Mock::AllowLeak(&rpc_client_for_broker_);

    ON_CALL(*rpc_client_for_broker_, needHeartbeat()).WillByDefault(testing::Return(true));

    ON_CALL(*rpc_client_for_broker_, ok()).WillByDefault(Invoke([this]() {
      std::cout << "Check ok()" << std::endl;
      return client_ok_;
    }));

    ON_CALL(*rpc_client_for_broker_, asyncQueryOffset(_, _))
        .WillByDefault(Invoke(std::bind(&DefaultMQPushConsumerUnitTest::mockQueryOffset, this, std::placeholders::_1,
                                        std::placeholders::_2)));

    ON_CALL(*rpc_client_for_broker_, asyncPull(_, _))
        .WillByDefault(Invoke(std::bind(&DefaultMQPushConsumerUnitTest::mockAsyncPull, this, std::placeholders::_1,
                                        std::placeholders::_2)));

    ON_CALL(*rpc_client_for_broker_, asyncHeartbeat(_, _))
        .WillByDefault(Invoke(std::bind(&DefaultMQPushConsumerUnitTest::mockHeartbeat, this, std::placeholders::_1,
                                        std::placeholders::_2)));

    ON_CALL(*rpc_client_for_broker_, asyncQueryAssignment(testing::_, testing::_))
        .WillByDefault(Invoke(std::bind(&DefaultMQPushConsumerUnitTest::mockQueryAssignment, this,
                                        std::placeholders::_1, std::placeholders::_2)));

    ON_CALL(*rpc_client_for_broker_, asyncReceive)
        .WillByDefault(testing::Invoke(std::bind(&DefaultMQPushConsumerUnitTest::mockReceiveMessage, this,
                                                 std::placeholders::_1, std::placeholders::_2)));

    const char* address_format = "ipv4:10.0.0.{}:10911";
    for (int i = 0; i < partition_num_ / avg_partition_per_host_; ++i) {
      std::string&& address = fmt::format(address_format, i);
      client_instance_->addRpcClient(address, rpc_client_for_broker_);
    }
  }

  void TearDown() override {
    rpc_client_for_broker_.reset();
    MQClientTest::TearDown();
  }

  void mockHeartbeat(const HeartbeatRequest& request, InvocationContext<HeartbeatResponse>* invocation_context) const {
    invocation_context->response.mutable_common()->mutable_status()->set_code(ok_);
    invocation_context->onCompletion(true);
  }

  void mockQueryOffset(const QueryOffsetRequest& request,
                       InvocationContext<QueryOffsetResponse>* invocation_context) const {
    invocation_context->response.mutable_common()->mutable_status()->set_code(ok_);
    invocation_context->response.set_offset(max_offset_);

    invocation_context->onCompletion(true);
  }

  void mockAsyncPull(const PullMessageRequest& request,
                     InvocationContext<PullMessageResponse>* invocation_context) const {
    invocation_context->response.mutable_common()->mutable_status()->set_code(ok_);
    invocation_context->response.set_next_offset(max_offset_);
    for (int i = 0; i < pull_batch_size_; i++) {
      auto message = new rmq::Message;
      message->mutable_topic()->set_arn(arn_);
      message->mutable_topic()->set_name(topic_);
      message->mutable_system_attribute()->set_message_id(std::to_string(++message_index_));
      message->set_body(body_);
      message->mutable_system_attribute()->set_body_encoding(rmq::Encoding::IDENTITY);
      message->mutable_system_attribute()->mutable_body_digest()->set_type(rmq::DigestType::MD5);
      std::string checksum;
      MixAll::md5(body_, checksum);
      message->mutable_system_attribute()->mutable_body_digest()->set_checksum(checksum);
      invocation_context->response.mutable_messages()->AddAllocated(message);
    }
    invocation_context->onCompletion(true);
  }

  void mockQueryAssignment(const QueryAssignmentRequest& request,
                           InvocationContext<QueryAssignmentResponse>* invocation_context) {
    for (int i = 0; i < 16; ++i) {
      auto load_assignment = new rmq::Assignment;
      load_assignment->set_mode(rmq::ConsumeMessageType::POP);
      load_assignment->mutable_partition()->set_id(i % 8);
      load_assignment->mutable_partition()->set_permission(rmq::Permission::READ_WRITE);
      load_assignment->mutable_partition()->mutable_topic()->set_name(request.topic().name());
      load_assignment->mutable_partition()->mutable_topic()->set_arn(request.topic().arn());
      std::string broker_name{"broker-"};
      broker_name.push_back('a' + i / 8);
      load_assignment->mutable_partition()->mutable_broker()->set_name(broker_name);
      load_assignment->mutable_partition()->mutable_broker()->set_id(0);
      auto endpoints = load_assignment->mutable_partition()->mutable_broker()->mutable_endpoints();
      endpoints->set_scheme(rmq::AddressScheme::IPv4);
      auto addresses = endpoints->mutable_addresses();
      auto address = new rmq::Address;
      address->set_host(fmt::format("10.0.0.{}", i / 8));
      address->set_port(10911);
      addresses->AddAllocated(address);
      invocation_context->response.mutable_assignments()->AddAllocated(load_assignment);
    }
    invocation_context->onCompletion(true);
  }

  void mockReceiveMessage(const ReceiveMessageRequest& request,
                          InvocationContext<ReceiveMessageResponse>* invocation_context) {
    invocation_context->response.mutable_common()->mutable_status()->set_code(google::rpc::Code::OK);

    // Set delivery timestamp
    auto since_unix_epoch = absl::Now() - absl::UnixEpoch();
    auto seconds = absl::ToInt64Seconds(since_unix_epoch);
    invocation_context->response.mutable_delivery_timestamp()->set_seconds(seconds);
    invocation_context->response.mutable_delivery_timestamp()->set_nanos(
        absl::ToInt64Nanoseconds(since_unix_epoch - absl::Seconds(seconds)));

    // Set invisible duration
    invocation_context->response.mutable_invisible_duration()->set_seconds(300);
    invocation_context->response.mutable_invisible_duration()->set_nanos(0);

    for (int32_t i = 0; i < request.batch_size(); i++) {
      auto message = new rmq::Message;
      message->set_body("Sample body");
    }

    invocation_context->onCompletion(true);
  }

  void
  mockForwardMessageToDeadLetterQueue(const ForwardMessageToDeadLetterQueueRequest& request,
                                      InvocationContext<ForwardMessageToDeadLetterQueueResponse>* invocation_context) {
    invocation_context->response.mutable_common()->mutable_status()->set_code(google::rpc::Code::OK);
    invocation_context->onCompletion(true);
  }

protected:
  std::shared_ptr<NiceMock<RpcClientMock>> rpc_client_for_broker_;
  const int64_t max_offset_{100};
  std::string name_server_address_{"127.0.0.1:9876"};
  int32_t pull_batch_size_{32};
  mutable int64_t message_index_{0};
  std::string body_{"Sample message body"};
};

std::atomic_bool completed_{false};
std::mutex completion_mtx_;
std::condition_variable completion_cv_;

class MessageListenerUnitTests : public StandardMessageListener {
public:
  ConsumeMessageResult consumeMessage(const std::vector<MQMessageExt>& msgs) override {
    if (!msgs.empty()) {
      std::unique_lock<std::mutex> lk(completion_mtx_);
      bool expected = false;
      if (completed_.compare_exchange_strong(expected, true)) {
        completion_cv_.notify_all();
        SPDLOG_INFO("Notify to stop unit test execution.");
      }
    }
    for (const auto& msg : msgs) {
      SPDLOG_INFO("Received a message[Topic={}, MessageId={}]", msg.getTopic(), msg.getMsgId());
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return ConsumeMessageResult::SUCCESS;
  }
};

TEST_F(DefaultMQPushConsumerUnitTest, testBroadcasting) {
  spdlog::set_level(spdlog::level::debug);
  auto push_consumer = std::make_shared<DefaultMQPushConsumerImpl>(group_name_);
  push_consumer->setMessageModel(MessageModel::BROADCASTING);
  push_consumer->arn(arn_);
  push_consumer->setNameServerList(name_server_list_);
  push_consumer->subscribe(topic_, "*");
  auto listener = new MessageListenerUnitTests;
  push_consumer->registerMessageListener(listener);
  push_consumer->start();
  {
    std::unique_lock<std::mutex> lk(completion_mtx_);
    completion_cv_.wait(lk, [&]() { return completed_.load(); });
  }
  push_consumer->shutdown();
  absl::SleepFor(absl::Seconds(10));
  delete listener;
}

TEST_F(DefaultMQPushConsumerUnitTest, DISABLED_testClustering) {
  spdlog::set_level(spdlog::level::debug);
  auto push_consumer = std::make_shared<DefaultMQPushConsumerImpl>(group_name_);
  push_consumer->setMessageModel(MessageModel::CLUSTERING);
  push_consumer->arn(arn_);
  push_consumer->setNameServerList(name_server_list_);
  push_consumer->subscribe(topic_, "*");

  auto listener = new MessageListenerUnitTests();
  push_consumer->registerMessageListener(listener);
  push_consumer->start();

  {
    std::unique_lock<std::mutex> lk(completion_mtx_);
    completion_cv_.wait(lk, [&]() { return completed_.load(); });
  }
  push_consumer->shutdown();
  delete listener;
}

ROCKETMQ_NAMESPACE_END