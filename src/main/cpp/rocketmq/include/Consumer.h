#pragma once

#include "Client.h"
#include "FilterExpression.h"
#include "ReceiveMessageAction.h"
#include "absl/container/flat_hash_map.h"
#include "absl/types/optional.h"

ROCKETMQ_NAMESPACE_BEGIN

class Consumer : virtual public Client {
public:
  ~Consumer() override = default;

  virtual absl::optional<FilterExpression>
  getFilterExpression(const std::string &topic) const = 0;

  virtual uint32_t maxCachedMessageQuantity() const = 0;

  virtual uint64_t maxCachedMessageMemory() const = 0;

  virtual int32_t receiveBatchSize() const = 0;

  virtual ReceiveMessageAction receiveMessageAction() const = 0;
};

ROCKETMQ_NAMESPACE_END