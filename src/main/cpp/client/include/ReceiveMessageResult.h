#pragma once

#include "MixAll.h"
#include "absl/time/time.h"
#include "rocketmq/MQMessageExt.h"

#include <cstdlib>
#include <sstream>
#include <utility>

ROCKETMQ_NAMESPACE_BEGIN

enum class ReceiveMessageStatus : int32_t {
  /**
   * Messages are received as expected.
   */
  OK,

  /**
   * Deadline expired before matched messages are found in the server side.
   */
  DEADLINE_EXCEEDED,

  /**
   * Resource has been exhausted, perhaps a per-user quota. For example, too many receive-message requests are submitted
   * to the same partition at the same time.
   */
  RESOURCE_EXHAUSTED,

  /**
   * The target partition does not exist, which might have been deleted.
   */
  NOT_FOUND,

  /**
   * Receive-message operation was attempted past the valid range. For pull operation, clients may try to pull expired
   * messages.
   */
  OUT_OF_RANGE,

  /**
   * Data is corrupted during transfer.
   */
  DATA_CORRUPTED,

  /**
   * Serious errors occurred in the server side.
   */
  INTERNAL
};

static const char* EnumStrings[] = {
    "FOUND", "DEADLINE_EXCEEDED", "RESOURCE_EXHAUSTED", "NOT_FOUND", "OUT_OF_RANGE", "DATA_CORRUPTED", "INTERNAL"};

struct ReceiveMessageResult {
  ReceiveMessageResult() = default;

  ReceiveMessageResult(ReceiveMessageStatus receive_status, absl::Time pop_time, absl::Duration invisible_period,
                       long long rest_num, std::vector<MQMessageExt> msg_found_list)
      : status_(receive_status), pop_time_(pop_time), invisible_time_(invisible_period),
        messages_(std::move(msg_found_list)) {}

  ReceiveMessageResult(const ReceiveMessageResult& other) {
    status_ = other.status_;
    pop_time_ = other.pop_time_;
    invisible_time_ = other.invisible_time_;
    messages_ = other.messages_;
    source_host_ = other.source_host_;
    next_offset_ = other.next_offset_;
  }

  ReceiveMessageResult& operator=(const ReceiveMessageResult& other) {
    if (this == &other) {
      return *this;
    }
    status_ = other.status_;
    pop_time_ = other.pop_time_;
    invisible_time_ = other.invisible_time_;
    messages_ = other.messages_;
    source_host_ = other.source_host_;
    next_offset_ = other.next_offset_;
    return *this;
  }

  ~ReceiveMessageResult() = default;

  std::string toString() const {
    std::stringstream ss;
    ss << "ReceiveMessageResult[ popStatus=" << EnumStrings[static_cast<int32_t>(status_)]
       << ", popTime=" << absl::FormatTime(pop_time_, absl::UTCTimeZone())
       << "[UTC], invisibleTime=" << ToInt64Milliseconds(invisible_time_)
       << "ms, len(msgFoundList)=" << messages_.size() << " ]";
    return ss.str();
  }

  void status(ReceiveMessageStatus receive_status) { status_ = receive_status; }

  ReceiveMessageStatus status() const { return status_; }

  void setPopTime(absl::Time pop_time) { pop_time_ = pop_time; }

  absl::Time getPopTime() const { return pop_time_; }

  void invisibleTime(absl::Duration invisible_period) { invisible_time_ = invisible_period; }

  absl::Duration invisibleTime() const { return invisible_time_; }

  void setMsgFoundList(std::vector<MQMessageExt>& msg_found_list) { messages_ = msg_found_list; }

  const std::vector<MQMessageExt>& getMsgFoundList() const { return messages_; }

  const std::string& sourceHost() const { return source_host_; }

  void sourceHost(std::string source_host) { source_host_ = std::move(source_host); }

  ReceiveMessageStatus status_{ReceiveMessageStatus::DEADLINE_EXCEEDED};

  absl::Time pop_time_;
  absl::Duration invisible_time_;

  std::vector<MQMessageExt> messages_;

  std::string source_host_;

  int64_t next_offset_{0};
};

ROCKETMQ_NAMESPACE_END