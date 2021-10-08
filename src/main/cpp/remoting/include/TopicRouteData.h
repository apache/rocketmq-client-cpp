#pragma once

#include <vector>

#include "BrokerData.h"
#include "QueueData.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

struct TopicRouteData {

  /**
   * @brief In Java, it's named "queueDatas"
   *
   */
  std::vector<QueueData> queue_data_;

  /**
   * @brief In Java, it's named "brokerDatas"
   *
   */
  std::vector<BrokerData> broker_data_;

  
  static TopicRouteData decode(const google::protobuf::Struct& root);
};

ROCKETMQ_NAMESPACE_END