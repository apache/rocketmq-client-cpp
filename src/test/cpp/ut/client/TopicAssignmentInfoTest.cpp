#include "TopicAssignmentInfo.h"
#include "rocketmq/ConsumeType.h"
#include "gtest/gtest.h"
#include <iostream>

ROCKETMQ_NAMESPACE_BEGIN

class QueryAssignmentInfoTest : public testing::Test {
protected:
  std::string resource_namespace_{"mq://test"};
  std::string topic_{"TopicTest"};
  std::string broker_name_{"broker-a"};
  int broker_id_ = 0;
  int total_ = 16;
};

TEST_F(QueryAssignmentInfoTest, testCtor) {
  QueryAssignmentResponse response;
  for (int i = 0; i < total_; i++) {
    auto assignment = new rmq::Assignment;
    assignment->mutable_partition()->mutable_topic()->set_resource_namespace(resource_namespace_);
    assignment->mutable_partition()->mutable_topic()->set_name(topic_);
    assignment->mutable_partition()->set_id(i);
    assignment->mutable_partition()->set_permission(rmq::Permission::READ);
    auto broker = assignment->mutable_partition()->mutable_broker();
    broker->set_name(broker_name_);
    broker->set_id(broker_id_);
    broker->mutable_endpoints()->set_scheme(rmq::AddressScheme::IPv4);

    auto address = new rmq::Address;
    address->set_host("10.0.0.1");
    address->set_port(10911);
    broker->mutable_endpoints()->mutable_addresses()->AddAllocated(address);
    response.mutable_assignments()->AddAllocated(assignment);
  }
  TopicAssignment assignment(response);
  EXPECT_EQ(total_, assignment.assignmentList().size());
  const auto& item = *assignment.assignmentList().begin();
  EXPECT_EQ(item.messageQueue().getBrokerName(), broker_name_);
  EXPECT_EQ(item.messageQueue().getTopic(), topic_);
  EXPECT_TRUE(item.messageQueue().getQueueId() < 16);
}

TEST_F(QueryAssignmentInfoTest, testCtor2) {
  QueryAssignmentResponse response;
  for (int i = 0; i < total_; i++) {
    auto assignment = new rmq::Assignment;
    assignment->mutable_partition()->mutable_topic()->set_resource_namespace(resource_namespace_);
    assignment->mutable_partition()->mutable_topic()->set_name(topic_);
    assignment->mutable_partition()->set_id(i);
    assignment->mutable_partition()->set_permission(rmq::Permission::READ_WRITE);
    auto broker = assignment->mutable_partition()->mutable_broker();
    broker->set_name(broker_name_);
    broker->set_id(broker_id_);
    broker->mutable_endpoints()->set_scheme(rmq::AddressScheme::IPv4);

    auto address = new rmq::Address;
    address->set_host("10.0.0.1");
    address->set_port(10911);
    broker->mutable_endpoints()->mutable_addresses()->AddAllocated(address);
    response.mutable_assignments()->AddAllocated(assignment);
  }
  TopicAssignment assignment(response);
  EXPECT_EQ(total_, assignment.assignmentList().size());
  const auto& item = *assignment.assignmentList().begin();
  EXPECT_EQ(item.messageQueue().getBrokerName(), broker_name_);
  EXPECT_EQ(item.messageQueue().getTopic(), topic_);
  EXPECT_TRUE(item.messageQueue().getQueueId() < 16);
}

TEST_F(QueryAssignmentInfoTest, testCtor3) {
  QueryAssignmentResponse response;
  for (int i = 0; i < total_; i++) {
    auto assignment = new rmq::Assignment;
    assignment->mutable_partition()->mutable_topic()->set_resource_namespace(resource_namespace_);
    assignment->mutable_partition()->mutable_topic()->set_name(topic_);
    assignment->mutable_partition()->set_id(i);
    assignment->mutable_partition()->set_permission(rmq::Permission::NONE);
    auto broker = assignment->mutable_partition()->mutable_broker();
    broker->set_name(broker_name_);
    broker->set_id(broker_id_);
    broker->mutable_endpoints()->set_scheme(rmq::AddressScheme::IPv4);

    auto address = new rmq::Address;
    address->set_host("10.0.0.1");
    address->set_port(10911);
    broker->mutable_endpoints()->mutable_addresses()->AddAllocated(address);
    response.mutable_assignments()->AddAllocated(assignment);
  }
  TopicAssignment assignment(response);
  EXPECT_TRUE(assignment.assignmentList().empty());
}

ROCKETMQ_NAMESPACE_END