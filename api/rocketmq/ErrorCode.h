#pragma once

#include "RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

constexpr int NO_TOPIC_ROUTE_INFO = -1;

constexpr int FAILED_TO_SELECT_MESSAGE_QUEUE = -2;

constexpr int FAILED_TO_RESOLVE_BROKER_ADDRESS_FROM_TOPIC_ROUTE = -3;

constexpr int FAILED_TO_SEND_MESSAGE = -4;

constexpr int FAILED_TO_POP_MESSAGE_ASYNCHRONOUSLY = -5;

constexpr int ILLEGAL_STATE = -6;

constexpr int MESSAGE_ILLEGAL = -7;

constexpr int BAD_CONFIGURATION = -8;

constexpr int MESSAGE_QUEUE_ILLEGAL = -9;

ROCKETMQ_NAMESPACE_END