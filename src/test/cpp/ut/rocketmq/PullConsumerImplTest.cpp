#include "PullConsumerImpl.h"
#include "ClientManagerFactory.h"
#include "ClientManagerMock.h"
#include "Scheduler.h"
#include "rocketmq/RocketMQ.h"
#include "gtest/gtest.h"
#include <memory>
#include <string>

ROCKETMQ_NAMESPACE_BEGIN

class PullConsumerImplTest : public testing::Test {
public:
  void SetUp() override {
    grpc_init();
    client_manager_ = std::make_shared<testing::NiceMock<ClientManagerMock>>();
    ON_CALL(*client_manager_, getScheduler).WillByDefault(testing::ReturnRef(scheduler_));
    ClientManagerFactory::getInstance().addClientManager(arn_, client_manager_);

    pull_consumer_ = std::make_shared<PullConsumerImpl>(group_);
    pull_consumer_->setNameServerList(name_server_list_);
    pull_consumer_->arn(arn_);

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
  std::string arn_{"arn:mq://test"};
  std::vector<std::string> name_server_list_{"10.0.0.1:9876"};
  std::string group_{"Group-0"};
  std::string topic_{"Test"};
  std::shared_ptr<testing::NiceMock<ClientManagerMock>> client_manager_;
  std::shared_ptr<PullConsumerImpl> pull_consumer_;
  Scheduler scheduler_;
  std::string broker_name_{"broker-a"};
  int broker_id_{0};
  std::string message_body_{"Message Body Content"};
  std::string broker_host_{"10.0.0.1"};
  int broker_port_{10911};
  int queue_id_{1};
  TopicRouteDataPtr topic_route_data_;
};

TEST_F(PullConsumerImplTest, testStartShutdown) {
  pull_consumer_->start();
  pull_consumer_->shutdown();
}

TEST_F(PullConsumerImplTest, testQueuesFor) {
  pull_consumer_->start();
  auto mock_resolve_route = [this](const std::string& target_host, const Metadata& metadata,
                                   const QueryRouteRequest& request, std::chrono::milliseconds timeout,
                                   const std::function<void(bool, const TopicRouteDataPtr& ptr)>& cb) {
    cb(true, topic_route_data_);
  };

  EXPECT_CALL(*client_manager_, resolveRoute)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_resolve_route));

  std::future<std::vector<MQMessageQueue>> future = pull_consumer_->queuesFor(topic_);
  auto queues = future.get();
  EXPECT_FALSE(queues.empty());
  pull_consumer_->shutdown();
}

ROCKETMQ_NAMESPACE_END