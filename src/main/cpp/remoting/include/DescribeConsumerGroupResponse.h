#pragma once

#include <string>
#include <vector>

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

struct DescribeConsumerGroupResponse {

  bool ok_{false};

  /**
   * @brief client-id list
   *
   */
  std::vector<std::string> members_;
};

ROCKETMQ_NAMESPACE_END