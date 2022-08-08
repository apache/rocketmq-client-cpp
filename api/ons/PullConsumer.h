#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "MessageQueueONS.h"
#include "ONSClientException.h"
#include "PullResultONS.h"

ONS_NAMESPACE_BEGIN

class ONSFactoryProperty;

class ONSCLIENT_API PullConsumer {
public:
  virtual ~PullConsumer() = default;

  virtual void start() = 0;

  virtual void shutdown() = 0;

  virtual void fetchSubscribeMessageQueues(const std::string& topic, std::vector<MessageQueueONS>& mqs) = 0;

  virtual PullResultONS pull(const MessageQueueONS& mq, const std::string& expression, std::int64_t offset,
                             std::int32_t batch_size) = 0;

  virtual std::int64_t searchOffset(const MessageQueueONS& mq, long long timestamp) = 0;

  virtual std::int64_t maxOffset(const MessageQueueONS& mq) = 0;

  virtual std::int64_t minOffset(const MessageQueueONS& mq) = 0;

  virtual void updateConsumeOffset(const MessageQueueONS& mq, std::int64_t offset) = 0;

  virtual void removeConsumeOffset(const MessageQueueONS& mq) = 0;

  virtual std::int64_t fetchConsumeOffset(const MessageQueueONS& mq, bool skip_cache) = 0;

  virtual void persistConsumerOffset4PullConsumer(const MessageQueueONS& mq) noexcept(false) = 0;
};

ONS_NAMESPACE_END