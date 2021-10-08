#include "QueueData.h"

ROCKETMQ_NAMESPACE_BEGIN

QueueData QueueData::decode(const google::protobuf::Struct& root) {
  auto fields = root.fields();

  QueueData queue_data;

  if (fields.contains("brokerName")) {
    queue_data.broker_name_ = fields["brokerName"].string_value();
  }

  if (fields.contains("readQueueNums")) {
    queue_data.read_queue_number_ = fields["readQueueNums"].number_value();
  }

  if (fields.contains("writeQueueNums")) {
    queue_data.write_queue_number_ = fields["writeQueueNums"].number_value();
  }

  if (fields.contains("perm")) {
    queue_data.perm_ = fields["perm"].number_value();
  }

  if (fields.contains("topicSynFlag")) {
    queue_data.topic_system_flag_ = fields["topicSynFlag"].number_value();
  }

  return queue_data;
}

ROCKETMQ_NAMESPACE_END