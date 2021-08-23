#include "ProducerImpl.h"
#include "ClientManagerFactory.h"
#include "ClientManagerMock.h"
#include "TopicRouteData.h"
#include "rocketmq/AsyncCallback.h"
#include "rocketmq/MQMessage.h"
#include "rocketmq/MQSelector.h"
#include "rocketmq/RocketMQ.h"
#include "rocketmq/SendResult.h"
#include <memory>

ROCKETMQ_NAMESPACE_BEGIN

class ProducerImplTest : public testing::Test {
public:
  ProducerImplTest()
      : credentials_provider_(std::make_shared<StaticCredentialsProvider>(access_key_, access_secret_)) {}

  void SetUp() override {
    grpc_init();
    client_manager_ = std::make_shared<testing::NiceMock<ClientManagerMock>>();
    ClientManagerFactory::getInstance().addClientManager(arn_, client_manager_);
    producer_ = std::make_shared<ProducerImpl>(group_);
    producer_->arn(arn_);
    producer_->setNameServerList(name_server_list_);
    producer_->setCredentialsProvider(credentials_provider_);

    {
      std::vector<Partition> partitions;
      Topic topic(arn_, topic_);
      std::vector<Address> broker_addresses{Address(broker_host_, broker_port_)};
      ServiceAddress service_address(AddressScheme::IPv4, broker_addresses);
      Broker broker(broker_name_, broker_id_, service_address);
      Partition partition(topic, queue_id_, Permission::READ_WRITE, broker);
      partitions.emplace_back(partition);
      std::string debug_string;
      topic_route_data_ = std::make_shared<TopicRouteData>(partitions, debug_string);
    }
  }

  void TearDown() override { grpc_shutdown(); }

protected:
  std::shared_ptr<testing::NiceMock<ClientManagerMock>> client_manager_;
  std::shared_ptr<ProducerImpl> producer_;
  std::vector<std::string> name_server_list_{"10.0.0.1:9876"};
  std::string arn_{"arn:mq://test"};
  std::string group_{"CID_test"};
  std::string topic_{"Topic0"};
  int queue_id_{1};
  std::string tag_{"TagA"};
  std::string key_{"key-0"};
  std::string message_group_{"group-0"};
  std::string broker_name_{"broker-a"};
  int broker_id_{0};
  std::string message_body_{"Message Body Content"};
  std::string broker_host_{"10.0.0.1"};
  int broker_port_{10911};
  TopicRouteDataPtr topic_route_data_;
  std::string access_key_{"access_key"};
  std::string access_secret_{"access_secret"};
  std::shared_ptr<CredentialsProvider> credentials_provider_;
};

TEST_F(ProducerImplTest, testStartShutdown) {
  Scheduler scheduler;
  ON_CALL(*client_manager_, getScheduler).WillByDefault(testing::ReturnRef(scheduler));
  producer_->start();
  producer_->shutdown();
}

TEST_F(ProducerImplTest, testSend) {
  Scheduler scheduler;
  ON_CALL(*client_manager_, getScheduler).WillByDefault(testing::ReturnRef(scheduler));
  auto mock_resolve_route = [this](const std::string& target_host, const Metadata& metadata,
                                   const QueryRouteRequest& request, std::chrono::milliseconds timeout,
                                   const std::function<void(bool, const TopicRouteDataPtr& ptr)>& cb) {
    cb(true, topic_route_data_);
  };

  EXPECT_CALL(*client_manager_, resolveRoute)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_resolve_route));

  bool cb_invoked = false;
  SendResult send_result;
  auto mock_send = [&](const std::string& target_host, const Metadata& metadata, SendMessageRequest& request,
                       SendCallback* cb) {
    cb->onSuccess(send_result);
    cb_invoked = true;
    return true;
  };

  EXPECT_CALL(*client_manager_, send).Times(testing::AtLeast(1)).WillRepeatedly(testing::Invoke(mock_send));
  producer_->start();

  MQMessage message(topic_, tag_, message_body_);
  producer_->send(message);
  EXPECT_TRUE(cb_invoked);
  producer_->shutdown();
}

TEST_F(ProducerImplTest, testSend_WithMessageGroup) {
  Scheduler scheduler;
  ON_CALL(*client_manager_, getScheduler).WillByDefault(testing::ReturnRef(scheduler));
  auto mock_resolve_route = [this](const std::string& target_host, const Metadata& metadata,
                                   const QueryRouteRequest& request, std::chrono::milliseconds timeout,
                                   const std::function<void(bool, const TopicRouteDataPtr& ptr)>& cb) {
    cb(true, topic_route_data_);
  };

  EXPECT_CALL(*client_manager_, resolveRoute)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_resolve_route));

  bool cb_invoked = false;
  SendResult send_result;
  auto mock_send = [&](const std::string& target_host, const Metadata& metadata, SendMessageRequest& request,
                       SendCallback* cb) {
    cb->onSuccess(send_result);
    cb_invoked = true;
    return true;
  };

  EXPECT_CALL(*client_manager_, send).Times(testing::AtLeast(1)).WillRepeatedly(testing::Invoke(mock_send));
  producer_->start();

  MQMessage message(topic_, tag_, message_body_);
  producer_->send(message, message_group_);
  EXPECT_TRUE(cb_invoked);
  producer_->shutdown();
}

class TestMessageQueueSelector : public MessageQueueSelector {
public:
  MQMessageQueue select(const std::vector<MQMessageQueue>& mqs, const MQMessage& msg, void* arg) override {
    return *mqs.begin();
  }
};

TEST_F(ProducerImplTest, testSend_WithMessageQueueSelector) {
  Scheduler scheduler;
  ON_CALL(*client_manager_, getScheduler).WillByDefault(testing::ReturnRef(scheduler));
  auto mock_resolve_route = [this](const std::string& target_host, const Metadata& metadata,
                                   const QueryRouteRequest& request, std::chrono::milliseconds timeout,
                                   const std::function<void(bool, const TopicRouteDataPtr& ptr)>& cb) {
    cb(true, topic_route_data_);
  };

  EXPECT_CALL(*client_manager_, resolveRoute)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_resolve_route));

  bool cb_invoked = false;
  SendResult send_result;
  auto mock_send = [&](const std::string& target_host, const Metadata& metadata, SendMessageRequest& request,
                       SendCallback* cb) {
    cb->onSuccess(send_result);
    cb_invoked = true;
    return true;
  };

  EXPECT_CALL(*client_manager_, send).Times(testing::AtLeast(1)).WillRepeatedly(testing::Invoke(mock_send));
  producer_->start();

  auto selector = absl::make_unique<TestMessageQueueSelector>();

  MQMessage message(topic_, tag_, message_body_);
  producer_->send(message, selector.get(), nullptr);
  EXPECT_TRUE(cb_invoked);
  producer_->shutdown();
}

class TestSendCallback : public SendCallback {
public:
  TestSendCallback(bool& completed, absl::Mutex& mtx, absl::CondVar& cv) : completed_(completed), mtx_(mtx), cv_(cv) {}
  void onSuccess(SendResult& send_result) override {
    absl::MutexLock lk(&mtx_);
    completed_ = true;
    cv_.SignalAll();
  }
  void onException(const MQException& e) override {
    absl::MutexLock lk(&mtx_);
    completed_ = true;
    cv_.SignalAll();
  }

protected:
  bool& completed_;
  absl::Mutex& mtx_;
  absl::CondVar& cv_;
};

TEST_F(ProducerImplTest, testAsyncSend) {
  Scheduler scheduler;
  ON_CALL(*client_manager_, getScheduler).WillByDefault(testing::ReturnRef(scheduler));
  auto mock_resolve_route = [this](const std::string& target_host, const Metadata& metadata,
                                   const QueryRouteRequest& request, std::chrono::milliseconds timeout,
                                   const std::function<void(bool, const TopicRouteDataPtr& ptr)>& cb) {
    cb(true, topic_route_data_);
  };

  EXPECT_CALL(*client_manager_, resolveRoute)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_resolve_route));

  bool cb_invoked = false;
  SendResult send_result;
  auto mock_send = [&](const std::string& target_host, const Metadata& metadata, SendMessageRequest& request,
                       SendCallback* cb) {
    cb->onSuccess(send_result);
    cb_invoked = true;
    return true;
  };

  EXPECT_CALL(*client_manager_, send).Times(testing::AtLeast(1)).WillRepeatedly(testing::Invoke(mock_send));

  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;

  auto send_callback = absl::make_unique<TestSendCallback>(completed, mtx, cv);

  producer_->start();

  MQMessage message(topic_, tag_, message_body_);
  producer_->send(message, send_callback.get());

  if (!completed) {
    absl::MutexLock lk(&mtx);
    cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
  }

  EXPECT_TRUE(cb_invoked);
  producer_->shutdown();
}

ROCKETMQ_NAMESPACE_END