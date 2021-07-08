#include "MessageSystemFlag.h"

namespace rocketmq {

const uint32_t MessageSystemFlag::BODY_COMPRESS_TYPE = 0x1;
const uint32_t MessageSystemFlag::MULTI_TAG_TYPE = 0x1 << 1;
const uint32_t MessageSystemFlag::PREPARED_TYPE = 0x1 << 2;
const uint32_t MessageSystemFlag::COMMIT_TYPE = 0x1 << 3;
const uint32_t MessageSystemFlag::ROLLBACK_TYPE = 0x3 << 2;

bool MessageSystemFlag::bodyCompressed(uint32_t flag) { return BODY_COMPRESS_TYPE == (flag & BODY_COMPRESS_TYPE); }

} // namespace rocketmq
