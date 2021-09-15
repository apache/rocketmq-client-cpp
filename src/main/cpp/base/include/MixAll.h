#pragma once

#include "absl/strings/string_view.h"
#include "re2/re2.h"
#include "rocketmq/MQMessage.h"

#include <chrono>
#include <cstdint>
#include <string>

ROCKETMQ_NAMESPACE_BEGIN

class MixAll {
public:
  static const int32_t MASTER_BROKER_ID;
  static const int32_t DEFAULT_RECEIVE_MESSAGE_BATCH_SIZE;
  static const uint32_t MAX_MESSAGE_BODY_SIZE;
  static const uint32_t MAX_CACHED_MESSAGE_COUNT;
  static const uint32_t DEFAULT_CACHED_MESSAGE_COUNT;
  static const uint64_t DEFAULT_CACHED_MESSAGE_MEMORY;
  static const uint32_t DEFAULT_CONSUME_THREAD_POOL_SIZE;
  static const uint32_t DEFAULT_CONSUME_MESSAGE_BATCH_SIZE;
  static const int32_t DEFAULT_MAX_DELIVERY_ATTEMPTS;

  static const RE2 TOPIC_REGEX;
  static const RE2 IP_REGEX;

  /**
   * The amount of time required before a popped message is eligible to be consumed again. By default, 30s.
   */
  static const std::chrono::duration<long long> DEFAULT_INVISIBLE_TIME_;

  static const std::chrono::duration<long long> PROCESS_QUEUE_EXPIRATION_THRESHOLD_;

  static const int32_t MAX_SEND_MESSAGE_ATTEMPT_TIMES_;

  static const std::string PROPERTY_TRANSACTION_PREPARED_;

  static const std::string DEFAULT_LOAD_BALANCER_STRATEGY_NAME_;

  static const uint32_t DEFAULT_COMPRESS_BODY_THRESHOLD_;

  static const char* HOME_PROFILE_ENV_;

  static const char* MESSAGE_KEY_SEPARATOR;

  // Tracing span name list
  static const char* SPAN_NAME_SEND_MESSAGE;
  static const char* SPAN_NAME_END_TRANSACTION;
  static const char* SPAN_NAME_AWAIT_CONSUMPTION;
  static const char* SPAN_NAME_CONSUME_MESSAGE;
  static const char* SPAN_NAME_PULL_MESSAGE;

  // Tracing attribute name list
  static const char* SPAN_ATTRIBUTE_ACCESS_KEY;
  static const char* SPAN_ATTRIBUTE_ARN;
  static const char* SPAN_ATTRIBUTE_TOPIC;
  static const char* SPAN_ATTRIBUTE_GROUP;
  static const char* SPAN_ATTRIBUTE_MESSAGE_ID;
  static const char* SPAN_ATTRIBUTE_TAG;
  static const char* SPAN_ATTRIBUTE_KEYS;
  static const char* SPAN_ATTRIBUTE_HOST;
  static const char* SPAN_ATTRIBUTE_MESSAGE_TYPE;
  static const char* SPAN_ATTRIBUTE_ATTEMPT_TIME;
  static const char* SPAN_ATTRIBUTE_DELIVERY_TIMESTAMP;
  static const char* SPAN_ATTRIBUTE_TRANSACTION_RESOLUTION;
  static const char* SPAN_ATTRIBUTE_AVAILABLE_TIMESTAMP;
  static const char* SPAN_ATTRIBUTE_BATCH_SIZE;

  // Tracing annotation
  static const char* SPAN_ANNOTATION_AWAIT_CONSUMPTION;
  static const char* SPAN_ANNOTATION_ATTR_START_TIME;

  template <typename Rep, typename Period> static int64_t millisecondsOf(std::chrono::duration<Rep, Period> duration) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  }

  template <typename Rep, typename Period> static int64_t microsecondsOf(std::chrono::duration<Rep, Period> duration) {
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
  }

  /**
   * Validate message is legal. Aka, topic
   * @param message
   * @return
   */
  static bool validate(const MQMessage& message);

  static uint32_t random(uint32_t left, uint32_t right);

  static bool crc32(const std::string& data, std::string& digest);

  static bool md5(const std::string& data, std::string& digest);

  static bool sha1(const std::string& data, std::string& digest);

  static std::string format(std::chrono::system_clock::time_point time_point);

  static std::string hex(const void* data, std::size_t len);

  static bool hexToBinary(const std::string& hex, std::vector<uint8_t>& bin);

  static bool homeDirectory(std::string& home);

  static bool isIPv4(absl::string_view host);

private:
  static bool hexCharValue(char c, uint8_t& value);
};

ROCKETMQ_NAMESPACE_END