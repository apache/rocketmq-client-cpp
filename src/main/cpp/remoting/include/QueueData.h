#pragma once

#include <cstdint>
#include <string>

#include "google/protobuf/struct.pb.h"
#include "google/protobuf/util/json_util.h"

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

struct QueueData {
  std::string broker_name_;
  std::int32_t read_queue_number_{0};
  std::int32_t write_queue_number_{0};
  std::uint32_t perm_{0};

  /**
   * @brief in Java, it's named "topicSynFlag"
   *
   */
  std::uint32_t topic_system_flag_{0};

  static QueueData decode(const google::protobuf::Struct& root);
};

ROCKETMQ_NAMESPACE_END