#pragma once

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

enum CommunicationMode
{
    ComMode_SYNC,
    ComMode_ASYNC,
    ComMode_ONEWAY
};

ROCKETMQ_NAMESPACE_END