#pragma once

#include <cstdint>

namespace rocketmq {
class MessageSystemFlag {
public:
  static const uint32_t BODY_COMPRESS_TYPE;
  static const uint32_t MULTI_TAG_TYPE;
  static const uint32_t PREPARED_TYPE;
  static const uint32_t COMMIT_TYPE;
  static const uint32_t ROLLBACK_TYPE;

  static bool bodyCompressed(uint32_t flag);

  template <typename T> static void clearFlag(T& value, T flag) { value &= ~flag; }

  template <typename T> static void tagFlag(T& value, T flag) {

#ifdef DEBUG
    // Ensure existing flags are orthogonal to the flag to tag
    T check = value & flag;
    if (0 != check && flag != check) {
      abort();
    }
#endif

    value |= flag;
  }
};
} // namespace rocketmq