#pragma once

#include "rocketmq/RocketMQ.h"
#include <cstdint>

ROCKETMQ_NAMESPACE_BEGIN

class RemotingConstants {
public:
  static const char NameValueSeparator;
  static const char PropertySeparator;
  static const char* KeySeparator;

  static const char* StartDeliveryTime;
  static const char* Keys;
  static const char* Tags;
  static const char* MessageId;
  static const char* DelayLevel;
  static const char* MessageGroup;

  static const std::uint32_t FlagCompression;
  static const std::uint32_t FlagTransactionPrepare;
  static const std::uint32_t FlagTransactionCommit;
  static const std::uint32_t FlagTransactionRollback;

  static const char* FilterTypeTag;
  static const char* FilterTypeSQL;

  static const std::int32_t MessageMagicCodeV1;
  static const std::int32_t MessageMagicCodeV2;
};

ROCKETMQ_NAMESPACE_END