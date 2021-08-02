#pragma once

#include "Client.h"
#include "FilterExpression.h"
#include "absl/container/flat_hash_map.h"

ROCKETMQ_NAMESPACE_BEGIN

class Consumer : virtual public Client {
public:
  ~Consumer() override = default;

  virtual absl::flat_hash_map<std::string, FilterExpression> getTopicFilterExpressionTable() const = 0;

  virtual uint32_t maxCachedMessageQuantity() const = 0;

  virtual uint64_t maxCachedMessageMemory() const = 0;

  virtual int32_t receiveBatchSize() const = 0;
};

ROCKETMQ_NAMESPACE_END