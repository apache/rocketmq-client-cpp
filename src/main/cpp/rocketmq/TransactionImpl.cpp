#include "TransactionImpl.h"
#include "ProducerImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

bool TransactionImpl::commit() {
  std::shared_ptr<ProducerImpl> producer = producer_.lock();
  if (!producer) {
    return false;
  }

  return producer->commit(message_id_, transaction_id_, endpoint_, trace_context_);
}

bool TransactionImpl::rollback() {
  std::shared_ptr<ProducerImpl> producer = producer_.lock();
  if (!producer) {
    return false;
  }
  return producer->rollback(message_id_, transaction_id_, endpoint_, trace_context_);
}

ROCKETMQ_NAMESPACE_END