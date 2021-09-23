#pragma once

#include <algorithm>
#include <vector>

#include "Partition.h"
#include "RpcClient.h"

ROCKETMQ_NAMESPACE_BEGIN

namespace rmq = apache::rocketmq::v1;

/**
 * Thread Safety: This class is immutable and thus effectively thread safe.
 */
class TopicRouteData {
public:
  TopicRouteData(std::vector<Partition> partitions, std::string debug_string)
      : partitions_(std::move(partitions)), debug_string_(std::move(debug_string)) {
    std::sort(partitions_.begin(), partitions_.end());
  }

  const std::vector<Partition>& partitions() const { return partitions_; }

  const std::string& debugString() const { return debug_string_; }

  bool operator==(const TopicRouteData& other) const { return partitions_ == other.partitions_; }

  bool operator!=(const TopicRouteData& other) const { return !this->operator==(other); }

private:
  std::vector<Partition> partitions_;
  std::string debug_string_;
};

using TopicRouteDataPtr = std::shared_ptr<TopicRouteData>;

ROCKETMQ_NAMESPACE_END