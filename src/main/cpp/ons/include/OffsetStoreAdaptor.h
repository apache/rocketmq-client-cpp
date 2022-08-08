#pragma once

#include <memory>

#include "ons/ONSClient.h"
#include "ons/OffsetStore.h"
#include "rocketmq/OffsetStore.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class OffsetStoreAdaptor : public ROCKETMQ_NAMESPACE::OffsetStore {
public:
  OffsetStoreAdaptor(std::unique_ptr<ONS_NAMESPACE::OffsetStore> store) : store_(std::move(store)) {
  }

  void load() override {
  }

  void updateOffset(const MQMessageQueue& message_queue, int64_t offset) override {
    store_->writeOffset(message_queue.simpleName(), offset);
  }

  bool readOffset(const MQMessageQueue& message_queue, int64_t& offset) override {
    return store_->readOffset(message_queue.simpleName(), offset);
  }

private:
  std::unique_ptr<ONS_NAMESPACE::OffsetStore> store_;
};

ROCKETMQ_NAMESPACE_END