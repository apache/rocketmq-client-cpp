#pragma once

#include "RpcClient.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

/**
 * This callback will be invoked to collect and prepare heartbeat data, which will be sent to brokers.
 */
class HeartbeatDataCallback {
public:
  virtual void onHeartbeatDataCallback(HeartbeatRequest& request) = 0;
};

ROCKETMQ_NAMESPACE_END