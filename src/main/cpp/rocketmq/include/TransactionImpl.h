#pragma once

#include <memory>
#include <string>

#include "rocketmq/Transaction.h"

ROCKETMQ_NAMESPACE_BEGIN

class ProducerImpl;

class TransactionImpl : public Transaction {
public:
  TransactionImpl(std::string message_id, std::string transaction_id, std::string endpoint, std::string trace_context,
                  const std::shared_ptr<ProducerImpl>& producer)
      : message_id_(std::move(message_id)), transaction_id_(std::move(transaction_id)), endpoint_(std::move(endpoint)),
        trace_context_(std::move(trace_context)), producer_(producer) {}

  ~TransactionImpl() override = default;

  bool commit() override;

  bool rollback() override;

  std::string messageId() const override;

  std::string transactionId() const override;

private:
  std::string message_id_;
  std::string transaction_id_;
  std::string endpoint_;
  std::string trace_context_;
  std::weak_ptr<ProducerImpl> producer_;
};

ROCKETMQ_NAMESPACE_END