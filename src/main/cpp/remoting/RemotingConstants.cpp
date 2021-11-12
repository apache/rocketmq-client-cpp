#include "RemotingConstants.h"

ROCKETMQ_NAMESPACE_BEGIN

const char RemotingConstants::NameValueSeparator = 1;
const char RemotingConstants::PropertySeparator = 2;
const char* RemotingConstants::KeySeparator = " ";

const char* RemotingConstants::StartDeliveryTime = "__STARTDELIVERTIME";
const char* RemotingConstants::Keys = "KEYS";
const char* RemotingConstants::Tags = "TAGS";
const char* RemotingConstants::MessageId = "UNIQ_KEY";
const char* RemotingConstants::DelayLevel = "DELAY";
const char* RemotingConstants::MessageGroup = "__SHARDINGKEY";

const std::uint32_t RemotingConstants::FlagCompression = 0x1;
const std::uint32_t RemotingConstants::FlagTransactionPrepare = 0x1 << 2;
const std::uint32_t RemotingConstants::FlagTransactionCommit = 0x2 << 2;
const std::uint32_t RemotingConstants::FlagTransactionRollback = 0x3 << 2;

ROCKETMQ_NAMESPACE_END