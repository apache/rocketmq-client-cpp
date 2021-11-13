#include "RemotingHelper.h"

#include <cstdint>
#include <string>

#include "absl/strings/numbers.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"

#include "LoggerImpl.h"
#include "RemotingConstants.h"

ROCKETMQ_NAMESPACE_BEGIN

absl::flat_hash_map<std::string, std::int64_t>
RemotingHelper::parseStartOffsetInfo(const std::string& start_offset_info) {
  absl::flat_hash_map<std::string, std::int64_t> result;
  if (start_offset_info.empty()) {
    return result;
  }

  std::vector<std::string> array;
  if (!absl::StrContains(start_offset_info, ';')) {
    array.push_back(start_offset_info);
  } else {
    array = absl::StrSplit(start_offset_info, ';');
  }

  for (const auto& item : array) {
    std::vector<std::string> split = absl::StrSplit(item, RemotingConstants::KeySeparator);
    if (split.size() != 3) {
      SPDLOG_WARN("Failed to parse startOffsetInfo: {}", item);
      continue;
    }

    std::string key = absl::StrJoin({split[0], split[1]}, "@");
    if (result.contains(key)) {
      SPDLOG_WARN("Failed to parse startOffsetInfo due to duplication. {}", start_offset_info);
      continue;
    }

    std::int64_t value;
    if (absl::SimpleAtoi(split[2], &value)) {
      result.insert({key, value});
    } else {
      SPDLOG_WARN("Failed to parse offset to number: {}", split[2]);
    }
  }
  return result;
}

absl::flat_hash_map<std::string, std::vector<std::int64_t>>
RemotingHelper::parseMsgOffsetInfo(const std::string& message_offset_info) {
  absl::flat_hash_map<std::string, std::vector<std::int64_t>> result;
  if (message_offset_info.empty()) {
    return result;
  }

  std::vector<std::string> array;

  if (!absl::StrContains(message_offset_info, ';')) {
    array.push_back(message_offset_info);
  } else {
    array = absl::StrSplit(message_offset_info, ';');
  }

  for (const auto& item : array) {
    std::vector<std::string> split = absl::StrSplit(item, RemotingConstants::KeySeparator);
    if (3 != split.size()) {
      SPDLOG_WARN("Failed to parse message offset map. Expecting 3 items after splitting with key-separator.[{}]",
                  item);
      continue;
    }

    std::string key = absl::StrJoin({split[0], split[1]}, "@");
    if (result.contains(key)) {
      SPDLOG_WARN("Failed to parse message offset map due to duplication.");
    }

    std::vector<std::int64_t> offsets;
    std::vector<absl::string_view> segments = absl::StrSplit(split[2], ',');
    for (auto item : segments) {
      std::int64_t offset;
      if (absl::SimpleAtoi(item, &offset)) {
        offsets.push_back(offset);
      }
    }
    result.insert({key, offsets});
  }
  return result;
}

absl::flat_hash_map<std::string, std::int32_t>
RemotingHelper::parseOrderCountInfo(const std::string& order_count_info) {
  absl::flat_hash_map<std::string, std::int32_t> result;
  if (order_count_info.empty()) {
    return result;
  }

  std::vector<std::string> array;
  if (!absl::StrContains(order_count_info, ';')) {
    array.push_back(order_count_info);
  } else {
    array = absl::StrSplit(order_count_info, ';');
  }

  for (const auto& item : array) {
    std::vector<std::string> segments = absl::StrSplit(item, RemotingConstants::KeySeparator);
    if (3 != segments.size()) {
      SPDLOG_WARN("Failed to parse order-count-info: {}", item);
      continue;
    }

    std::string key = absl::StrJoin({segments[0], segments[1]}, "@");
    if (result.contains(key)) {
      SPDLOG_WARN("Failed to parse order-count-info due to duplication key: {}", key);
      continue;
    }

    std::int32_t offset = 0;
    if (absl::SimpleAtoi(segments[2], &offset)) {
      result.insert({key, offset});
    }
  }
  return result;
}

absl::flat_hash_map<std::string, std::string> RemotingHelper::stringToMessageProperties(absl::string_view properties) {
  absl::flat_hash_map<std::string, std::string> result;
  std::vector<absl::string_view> entries = absl::StrSplit(properties, RemotingConstants::PropertySeparator);
  for (const auto& entry : entries) {
    std::vector<std::string> kv = absl::StrSplit(entry, RemotingConstants::NameValueSeparator);
    if (kv.size() == 2) {
      result.emplace(kv[0], kv[1]);
    }
  }
  return result;
}

ROCKETMQ_NAMESPACE_END