#include "MessageVersion.h"
#include "RemotingConstants.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

MessageVersion messageVersionOf(std::int32_t magic_code) {
  if (RemotingConstants::MessageMagicCodeV1 == magic_code) {
    return MessageVersion::V1;
  }

  if (RemotingConstants::MessageMagicCodeV2 == magic_code) {
    return MessageVersion::V2;
  }

  return MessageVersion::Unset;
}

ROCKETMQ_NAMESPACE_END