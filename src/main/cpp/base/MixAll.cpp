#include "MixAll.h"

#include <chrono>

#include "absl/random/random.h"
#include "absl/strings/str_split.h"

#include "fmt/format.h"

#include <openssl/md5.h>
#include <openssl/sha.h>

#include "zlib.h"
#include <arpa/inet.h>
#include <pwd.h>
#include <unistd.h>

ROCKETMQ_NAMESPACE_BEGIN

const int32_t MixAll::MASTER_BROKER_ID = 0;

const int32_t MixAll::DEFAULT_RECEIVE_MESSAGE_BATCH_SIZE = 32;

const uint32_t MixAll::MAX_MESSAGE_BODY_SIZE = 1024 * 1024 * 4;
const uint32_t MixAll::MAX_CACHED_MESSAGE_COUNT = 65535;
const uint32_t MixAll::DEFAULT_CACHED_MESSAGE_COUNT = 1024;
const uint64_t MixAll::DEFAULT_CACHED_MESSAGE_MEMORY = 128L * 1024 * 1024;
const uint32_t MixAll::DEFAULT_CONSUME_THREAD_POOL_SIZE = 20;
const uint32_t MixAll::DEFAULT_CONSUME_MESSAGE_BATCH_SIZE = 1;
const int32_t MixAll::DEFAULT_MAX_DELIVERY_ATTEMPTS = 16;

const RE2 MixAll::TOPIC_REGEX("[a-zA-Z0-9\\-_]{3,64}");

const std::chrono::duration<long long> MixAll::DEFAULT_INVISIBLE_TIME_ = std::chrono::seconds(30);

const std::chrono::duration<long long> MixAll::PROCESS_QUEUE_EXPIRATION_THRESHOLD_ = std::chrono::seconds(120);

const int32_t MixAll::MAX_SEND_MESSAGE_ATTEMPT_TIMES_ = 3;

const std::string MixAll::PROPERTY_TRANSACTION_PREPARED_ = "TRAN_MSG";

const std::string MixAll::DEFAULT_LOAD_BALANCER_STRATEGY_NAME_ = "AVG";

const uint32_t MixAll::DEFAULT_COMPRESS_BODY_THRESHOLD_ = 1024 * 1024 * 4;

const char* MixAll::HOME_PROFILE_ENV_ = "HOME";
const char* MixAll::MESSAGE_KEY_SEPARATOR = " ";

// Span name list
const char* MixAll::SPAN_NAME_SEND_MESSAGE = "SendMessage";
const char* MixAll::SPAN_NAME_END_TRANSACTION = "EndTransaction";
const char* MixAll::SPAN_NAME_AWAIT_CONSUMPTION = "AwaitingConsumption";
const char* MixAll::SPAN_NAME_CONSUME_MESSAGE = "ConsumeMessage";
const char* MixAll::SPAN_NAME_PULL_MESSAGE = "PullMessage";

// Span attribute name list
const char* MixAll::SPAN_ATTRIBUTE_ACCESS_KEY = "ak";
const char* MixAll::SPAN_ATTRIBUTE_ARN = "arn";
const char* MixAll::SPAN_ATTRIBUTE_KEYS = "keys";
const char* MixAll::SPAN_ATTRIBUTE_MESSAGE_TYPE = "msgType";
const char* MixAll::SPAN_ATTRIBUTE_DELIVERY_TIMESTAMP = "deliveryTimestamp";
const char* MixAll::SPAN_ATTRIBUTE_TOPIC = "topic";
const char* MixAll::SPAN_ATTRIBUTE_GROUP = "group";
const char* MixAll::SPAN_ATTRIBUTE_MESSAGE_ID = "msgId";
const char* MixAll::SPAN_ATTRIBUTE_TAG = "tags";
const char* MixAll::SPAN_ATTRIBUTE_HOST = "host";
const char* MixAll::SPAN_ATTRIBUTE_ATTEMPT_TIME = "attempt";
const char* MixAll::SPAN_ATTRIBUTE_TRANSACTION_RESOLUTION = "commitAction";
const char* MixAll::SPAN_ATTRIBUTE_AVAILABLE_TIMESTAMP = "availableTimestamp";
const char* MixAll::SPAN_ATTRIBUTE_BATCH_SIZE = "batchSize";

// Span annotation
const char* MixAll::SPAN_ANNOTATION_AWAIT_CONSUMPTION = "__await_consumption";
const char* MixAll::SPAN_ANNOTATION_ATTR_START_TIME = "__start_time";

bool MixAll::validate(const MQMessage& message) {
  if (message.getTopic().empty()) {
    return false;
  }
  const std::string& topic = message.getTopic();
  // Topic should not start with "CID" or "GID" which are reserved prefix
  if (absl::StartsWith(topic, "CID") || absl::StartsWith(topic, "GID")) {
    return false;
  }

  // Legal topic characters are a-z, A-Z, 0-9, hyphen('-') and underline('_')
  if (!RE2::FullMatch(topic, TOPIC_REGEX)) {
    return false;
  }

  uint32_t body_length = message.bodyLength();
  if (!body_length || body_length > MAX_MESSAGE_BODY_SIZE) {
    return false;
  }
  return true;
}

uint32_t MixAll::random(uint32_t left, uint32_t right) {
  static absl::BitGen gen;
  return absl::Uniform(gen, left, right);
}

bool MixAll::crc32(const std::string& data, std::string& digest) {
  uLong crc = ::crc32(0L, reinterpret_cast<const Bytef*>(data.c_str()), data.length());
  uint32_t network_byte_order = htonl(crc);
  digest = hex(&network_byte_order, sizeof(network_byte_order));
  return true;
}

bool MixAll::md5(const std::string& data, std::string& digest) {
  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, data.data(), data.length());
  unsigned char md[MD5_DIGEST_LENGTH + 1];
  int success = MD5_Final(md, &ctx);
  if (!success) {
    return false;
  }
  digest.clear();
  digest.append(hex(md, MD5_DIGEST_LENGTH));
  return true;
}

bool MixAll::sha1(const std::string& data, std::string& digest) {
  unsigned char out[SHA_DIGEST_LENGTH];
  SHA_CTX ctx;
  SHA1_Init(&ctx);
  SHA1_Update(&ctx, data.data(), data.length());
  SHA1_Final(out, &ctx);
  digest.clear();
  digest.append(hex(reinterpret_cast<const char*>(out), SHA_DIGEST_LENGTH));
  return true;
}

std::string MixAll::format(std::chrono::system_clock::time_point time_point) {
  std::time_t creation_time_t = std::chrono::system_clock::to_time_t(time_point);
  auto fraction = std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count() % 1000;
  char fmt_date_time[128];

  /**
   * TODO: std::localtime is not thread-safe, output, as a result, may be less reliable in highly contending
   * scenario
   */
  std::strftime(fmt_date_time, sizeof(fmt_date_time), "%Y-%m-%d %H:%M:%S", std::localtime(&creation_time_t));
  return fmt::format("{}.{}", fmt_date_time, fraction);
}

std::string MixAll::hex(const void* data, std::size_t len) {
  const char dict[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  std::string s;
  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data);
  for (std::size_t i = 0; i < len; i++) {
    unsigned char c = *(ptr + i);
    s.append(&dict[(0xF0 & c) >> 4], 1);
    s.append(&dict[(0x0F & c)], 1);
  }
  return s;
}

bool MixAll::hexToBinary(const std::string& hex, std::vector<uint8_t>& bin) {
  // Length of valid Hex string should always be even.
  if (hex.length() % 2) {
    return false;
  }
  for (std::string::size_type i = 0; i < hex.length(); i += 2) {
    char c1 = hex.at(i);
    char c2 = hex.at(i + 1);

    uint8_t value = 0;
    uint8_t tmp;
    if (hexCharValue(c1, tmp)) {
      value = tmp << 4;
    } else {
      return false;
    }

    if (hexCharValue(c2, tmp)) {
      value |= tmp;
      bin.push_back(value);
    } else {
      return false;
    }
  }
  return true;
}

bool MixAll::hexCharValue(char c, uint8_t& value) {
  if ('0' <= c && c <= '9') {
    value = c - '0';
    return true;
  }

  if ('a' <= c && c <= 'f') {
    value = c - 'a' + 10;
    return true;
  }

  if ('A' <= c && c <= 'F') {
    value = c - 'A' + 10;
    return true;
  }

  return false;
}

bool MixAll::homeDirectory(std::string& home_dir) {
#ifndef _WIN32
  char* home = getenv(HOME_PROFILE_ENV_);
  if (home) {
    home_dir.append(home, strlen(home));
    return true;
  } else {
    struct passwd* pwd = getpwuid(getuid());
    if (pwd) {
      home_dir.clear();
      home_dir.append(pwd->pw_dir, strlen(pwd->pw_dir));
      return true;
    }
  }
  return false;
#endif
}

ROCKETMQ_NAMESPACE_END