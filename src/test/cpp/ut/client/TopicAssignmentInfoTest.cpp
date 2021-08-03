#include "TopicAssignmentInfo.h"
#include "rocketmq/ConsumeType.h"
#include "gtest/gtest.h"
#include <iostream>

ROCKETMQ_NAMESPACE_BEGIN

class QueryAssignmentInfoTest : public testing::Test {
protected:
  std::string arn{"arn:mq://test"};
  std::string topic{"TopicTest"};
  std::string broker_name{"broker-a"};
  int broker_id = 0;
  int total = 16;
};

TEST_F(QueryAssignmentInfoTest, testCtor) {
  QueryAssignmentResponse response;
  for (int i = 0; i < total; i++) {
    auto assignment = new rmq::Assignment;
    assignment->set_mode(rmq::ConsumeMessageType::POP);
    assignment->mutable_partition()->mutable_topic()->set_arn(arn);
    assignment->mutable_partition()->mutable_topic()->set_name(topic);
    assignment->mutable_partition()->set_id(i);
    assignment->mutable_partition()->set_permission(rmq::Permission::READ);
    auto broker = assignment->mutable_partition()->mutable_broker();
    broker->set_name(broker_name);
    broker->set_id(broker_id);
    broker->mutable_endpoints()->set_scheme(rmq::AddressScheme::IPv4);

    auto address = new rmq::Address;
    address->set_host("10.0.0.1");
    address->set_port(10911);
    broker->mutable_endpoints()->mutable_addresses()->AddAllocated(address);
    response.mutable_assignments()->AddAllocated(assignment);
  }
  TopicAssignment assignment(response);
  EXPECT_EQ(total, assignment.assignmentList().size());
  const auto& item = *assignment.assignmentList().begin();
  EXPECT_EQ(item.messageQueue().getBrokerName(), broker_name);
  EXPECT_EQ(item.messageQueue().getTopic(), topic);
  EXPECT_EQ(item.consumeType(), ConsumeMessageType::POP);
  EXPECT_TRUE(item.messageQueue().getQueueId() < 16);
}

TEST_F(QueryAssignmentInfoTest, testCtor2) {
  QueryAssignmentResponse response;
  for (int i = 0; i < total; i++) {
    auto assignment = new rmq::Assignment;
    assignment->set_mode(rmq::ConsumeMessageType::POP);
    assignment->mutable_partition()->mutable_topic()->set_arn(arn);
    assignment->mutable_partition()->mutable_topic()->set_name(topic);
    assignment->mutable_partition()->set_id(i);
    assignment->mutable_partition()->set_permission(rmq::Permission::READ_WRITE);
    auto broker = assignment->mutable_partition()->mutable_broker();
    broker->set_name(broker_name);
    broker->set_id(broker_id);
    broker->mutable_endpoints()->set_scheme(rmq::AddressScheme::IPv4);

    auto address = new rmq::Address;
    address->set_host("10.0.0.1");
    address->set_port(10911);
    broker->mutable_endpoints()->mutable_addresses()->AddAllocated(address);
    response.mutable_assignments()->AddAllocated(assignment);
  }
  TopicAssignment assignment(response);
  EXPECT_EQ(total, assignment.assignmentList().size());
  const auto& item = *assignment.assignmentList().begin();
  EXPECT_EQ(item.messageQueue().getBrokerName(), broker_name);
  EXPECT_EQ(item.messageQueue().getTopic(), topic);
  EXPECT_EQ(item.consumeType(), ConsumeMessageType::POP);
  EXPECT_TRUE(item.messageQueue().getQueueId() < 16);
}

TEST_F(QueryAssignmentInfoTest, testCtor3) {
  QueryAssignmentResponse response;
  for (int i = 0; i < total; i++) {
    auto assignment = new rmq::Assignment;
    assignment->set_mode(rmq::ConsumeMessageType::POP);
    assignment->mutable_partition()->mutable_topic()->set_arn(arn);
    assignment->mutable_partition()->mutable_topic()->set_name(topic);
    assignment->mutable_partition()->set_id(i);
    assignment->mutable_partition()->set_permission(rmq::Permission::NONE);
    auto broker = assignment->mutable_partition()->mutable_broker();
    broker->set_name(broker_name);
    broker->set_id(broker_id);
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