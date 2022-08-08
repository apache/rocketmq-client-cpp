#pragma once

#include "rocketmq/MQMessageQueue.h"
#include "rocketmq/MQSelector.h"
#include "rocketmq/RocketMQ.h"

#include "ons/ONSClient.h"

ONS_NAMESPACE_BEGIN

class ONSMessageQueueSelector : public ROCKETMQ_NAMESPACE::MessageQueueSelector {
public:
  ROCKETMQ_NAMESPACE::MQMessageQueue select(const std::vector<ROCKETMQ_NAMESPACE::MQMessageQueue>& mqs,
                                            const ROCKETMQ_NAMESPACE::MQMessage& msg, void* arg) override;
};

ONS_NAMESPACE_END