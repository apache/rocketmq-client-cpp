#pragma once

#include "rocketmq/RocketMQ.h"

#include <cstdint>

ROCKETMQ_NAMESPACE_BEGIN

enum class ReceiveMessageAction : std::int8_t {
  /**
   * @brief Use Receive and Ack strategy. Progress of message consumption is
   * managed by the server.
   */
  POLLING = 0,

  /**
   * @brief A consumer client dynamically binds one or multiple message queues
   * through user-defined-allocation algorithm. The client then range-scans the
   * specified message queue and selectively ack through persisting offset to
   * brokers.
   */
  PULL = 1,
};

ROCKETMQ_NAMESPACE_END