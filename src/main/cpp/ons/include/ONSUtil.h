#pragma once

#include "absl/container/flat_hash_set.h"

#include "ons/Message.h"
#include "rocketmq/Logger.h"
#include "rocketmq/MQMessageExt.h"
#include "rocketmq/RocketMQ.h"
#include "spdlog/spdlog.h"

ONS_NAMESPACE_BEGIN

class ONSUtil {
public:
  ONSUtil();

  static ONSUtil& get();

  /**
   * @brief This function translates messages from RocketMQ representation back to ONS. It should be employed during
   * message consumption procedure.
   *
   * @param message_ext
   * @return Message
   */
  Message msgConvert(const ROCKETMQ_NAMESPACE::MQMessageExt& message_ext);

  /**
   * @brief This function translates ONS message to RocketMQ counterpart. It should be used when delivering messages to
   * brokers.
   *
   * @param message
   * @return ROCKETMQ_NAMESPACE::MQMessage
   */
  ROCKETMQ_NAMESPACE::MQMessage msgConvert(const Message& message);

private:
  absl::flat_hash_set<std::string> reserved_key_set_ext_;
};

ONS_NAMESPACE_END