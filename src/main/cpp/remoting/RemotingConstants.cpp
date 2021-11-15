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
const char* RemotingConstants::PopCk = "POP_CK";

const std::uint32_t RemotingConstants::FlagCompression = 0x1;
const std::uint32_t RemotingConstants::FlagTransactionPrepare = 0x1 << 2;
const std::uint32_t RemotingConstants::FlagTransactionCommit = 0x2 << 2;
const std::uint32_t RemotingConstants::FlagTransactionRollback = 0x3 << 2;

const char* RemotingConstants::FilterTypeTag = "TAG";
const char* RemotingConstants::FilterTypeSQL = "SQL92";

const std::int32_t RemotingConstants::MessageMagicCodeV1 = 0xAABBCCDD ^ (1880681586 + 8);
const std::int32_t RemotingConstants::MessageMagicCodeV2 = 0xAABBCCDD ^ (1880681586 + 4);

const char* RemotingConstants::RetryTopicPrefix = "%RETRY%";
const char* RemotingConstants::DlqTopicPrefix = "%DLQ%";

ROCKETMQ_NAMESPACE_END