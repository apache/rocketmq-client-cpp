#pragma once

#include <string>

#include "RemotingConstants.h"
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
};

ROCKETMQ_NAMESPACE_END