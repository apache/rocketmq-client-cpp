#pragma once

#include "RpcClient.h"

namespace rocketmq {

/**
 * This callback will be invoked to collect and prepare heartbeat data, which will be sent to brokers.
 */
class HeartbeatDataCallback {
public:
  virtual void onHeartbeatDataCallback(HeartbeatRequest& request) = 0;
};
} // namespace rocketmq