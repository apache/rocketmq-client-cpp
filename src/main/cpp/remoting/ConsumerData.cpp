#include "ConsumerData.h"
#include "CommandCustomHeader.h"
#include "rocketmq/MessageModel.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

bool operator==(const ConsumerData& lhs, const ConsumerData& rhs) {
  return lhs.group_name_ == rhs.group_name_ && lhs.consume_type_ == rhs.consume_type_ &&
         lhs.message_model_ == rhs.message_model_ && lhs.consume_from_where_ == rhs.consume_from_where_;
}

void ConsumerData::encode(google::protobuf::Struct& root) const {
  auto fields = root.mutable_fields();
  addEntry(fields, "groupName", group_name_);
  switch (consume_type_) {
    case ConsumeType::ConsumeActively: {
      addEntry(fields, "consumeType", "PULL");
      break;
    }
    case ConsumeType::ConsumePassively: {
      addEntry(fields, "consumeType", "PUSH");
      break;
    }
    case ConsumeType::ConsumePop: {
      addEntry(fields, "consumeType", "POP");
      break;
    }
    default: {
      break;
    }
  }

  switch (message_model_) {
    case MessageModel::BROADCASTING: {
      addEntry(fields, "messageModel", "BROADCASTING");
    }
    case MessageModel::CLUSTERING: {
      addEntry(fields, "messageModel", "CLUSTERING");
      break;
    }
  }

  switch (consume_from_where_) {
    case ConsumeFromWhere::ConsumeFromLastOffset: {
      addEntry(fields, "consumeFromWhere", "CONSUME_FROM_LAST_OFFSET");
      break;
    }
    case ConsumeFromWhere::ConsumeFromFirstOffset: {
      addEntry(fields, "consumeFromWhere", "CONSUME_FROM_FIRST_OFFSET");
      break;
    }
    case ConsumeFromWhere::ConsumeFromTimestamp: {
      addEntry(fields, "consumeFromWhere", "CONSUME_FROM_TIMESTAMP");
      break;
    }
  }

  {
    google::protobuf::Value sub_set;
    for (const auto& item : subscription_data_set_) {
      auto node = new google::protobuf::Value;
      item.encode(*node->mutable_struct_value());
      sub_set.mutable_list_value()->mutable_values()->AddAllocated(node);
    }
    fields->insert({"subscriptionDataSet", sub_set});
  }

  addEntry(fields, "unitMode", unit_mode_);
}

ROCKETMQ_NAMESPACE_END