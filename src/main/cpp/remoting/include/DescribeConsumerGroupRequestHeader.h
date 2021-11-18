#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class DescribeConsumerGroupRequestHeader : public CommandCustomHeader {
public:
  void encode(google::protobuf::Value& root) const override {
    auto fields = root.mutable_struct_value()->mutable_fields();
    addEntry(fields, "consumerGroup", group_);
  }

  std::string group_;
};

ROCKETMQ_NAMESPACE_END