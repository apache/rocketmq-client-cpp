#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "absl/time/clock.h"
#include "absl/time/time.h"

#include "DigestType.h"
#include "Encoding.h"
#include "rocketmq/MessageType.h"

ROCKETMQ_NAMESPACE_BEGIN

class Protocol {
public:
  static const char* PROTOCOL_VERSION;
};

struct Digest {
  DigestType digest_type{DigestType::MD5};
  std::string checksum;
  Digest() = default;
};

struct Resource {
  /**
   * Abstract resource namespace
   */
  std::string resource_namespace;

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
