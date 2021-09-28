#include "TransactionImpl.h"
#include "ProducerImpl.h"
#include <string>

ROCKETMQ_NAMESPACE_BEGIN

bool TransactionImpl::commit() {
  std::shared_ptr<ProducerImpl> producer = producer_.lock();
  if (!producer) {
    return false;
  }

  return producer->commit(message_id_, transaction_id_, trace_context_, endpoint_);
}

bool TransactionImpl::rollback() {
  std::shared_ptr<ProducerImpl> producer = producer_.lock();
  if (!producer) {
    return false;
  }
  return producer->rollback(message_id_, transaction_id_, trace_context_, endpoint_);
}

std::string TransactionImpl::messageId() const {
  return message_id_;
}

std::string TransactionImpl::transactionId() const {
  return transaction_id_;
}

ROCKETMQ_NAMESPACE_END