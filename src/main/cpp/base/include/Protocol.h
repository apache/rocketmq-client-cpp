#pragma once

#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "rocketmq/RocketMQ.h"
#include <chrono>
#include <string>
#include <vector>

ROCKETMQ_NAMESPACE_BEGIN

class Protocol {
public:
  static const char* PROTOCOL_VERSION;
};

enum class MessageType : int8_t {
  NORMAL = 0,
  FIFO = 1,
  DELAY = 2,
  TRANSACTION = 3,
};

enum class DigestType : int8_t {
  CRC32 = 0,
  MD5 = 1,
  SHA1 = 2,
};

struct Digest {
  DigestType digest_type{DigestType::MD5};
  std::string checksum;
  Digest() = default;
};

enum class Encoding : int8_t {
  IDENTITY = 0,
  GZIP = 1,
  SNAPPY = 2,
};

struct Resource {
  /**
   * Abstract resource namespace
   */
  std::string arn;

  /**
   * Resource name, which remains unique within given abstract resource namespace.
   */
  std::string name;
};

struct SystemAttribute {
  std::string tag;
  std::vector<std::string> keys;
  std::string message_id;
  Digest digest;
  Encoding body_encoding;
  MessageType message_type;
  absl::Time born_timestamp{absl::Now()};
  std::string born_host;
  absl::Time store_timestamp{absl::UnixEpoch()};
  std::string store_host;
  absl::Time delivery_timestamp{absl::UnixEpoch()};
  absl::Time decode_timestamp{absl::Now()};
  int32_t delay_level{0};
  std::string receipt_handle;
  int32_t partition_id{0};
  int64_t partition_offset{0};
  absl::Duration invisible_period;
  int32_t attempt_times{0};
  Resource publisher_group;
  std::string trace_context;
  std::string target_endpoint;
};

ROCKETMQ_NAMESPACE_END
