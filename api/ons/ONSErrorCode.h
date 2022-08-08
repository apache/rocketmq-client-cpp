#pragma once

#include "ONSClient.h"

ONS_NAMESPACE_BEGIN

constexpr int SEND_CALLBACK_IS_EMPTY = -1;
constexpr int MESSAGE_LISTENER_IS_EMPTY = -2;
constexpr int MESSAGE_SELECTOR_QUEUE_EMPTY = -3;
constexpr int CONSUME_MESSAGE_LISTENER_IS_NULL = -4;
constexpr int OTHER_ERROR = -999;

ONS_NAMESPACE_END