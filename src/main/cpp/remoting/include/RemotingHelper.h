#pragma once

#include <cstdint>
#include <string>
#include <system_error>

#include "RemotingConstants.h"
#include "absl/base/internal/endian.h"
#include "absl/container/flat_hash_map.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class RemotingHelper {
public:
  template <typename Map>
  static std::string messagePropertiesToString(const Map& properties) {
    std::string result;
    for (const auto& entry : properties) {
      if (!result.empty()) {
        result.append(1, RemotingConstants::PropertySeparator);
        result.insert(result.end(), entry.first.begin(), entry.first.end());
        result.append(1, RemotingConstants::NameValueSeparator);
        result.insert(result.end(), entry.second.begin(), entry.second.end());
      }
    }
    return result;
  }

  /**
   * @brief Craps to adapt to existing Remoting#pop API
   *
   * @param start_offset_info
   * @return absl::flat_hash_map<std::string, std::int64_t>
   */
  static absl::flat_hash_map<std::string, std::int64_t> parseStartOffsetInfo(const std::string& start_offset_info);

  static absl::flat_hash_map<std::string, std::vector<std::int64_t>>
  parseMsgOffsetInfo(const std::string& message_offset_info);

  static absl::flat_hash_map<std::string, std::int32_t> parseOrderCountInfo(const std::string& order_count_info);

  template <typename T>
  static T readBigEndian(const std::uint8_t* ptr, std::error_code& ec) {
    const T* p = reinterpret_cast<const T*>(ptr);
    if (sizeof(T) == 2) {
      return absl::gntohs(*p);
    } else if (sizeof(T) == 4) {
      return absl::gntohl(*p);
    } else if (sizeof(T) == 8) {
      return absl::gntohll(*p);
    } else {
      ec = std::make_error_code(std::errc::invalid_argument);
      return *p;
    }
  }

  static absl::flat_hash_map<std::string, std::string> stringToMessageProperties(absl::string_view properties);
};

ROCKETMQ_NAMESPACE_END