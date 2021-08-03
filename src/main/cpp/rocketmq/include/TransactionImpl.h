#pragma once

#include "rocketmq/Transaction.h"

#include <memory>
#include <string>

ROCKETMQ_NAMESPACE_BEGIN

class ProducerImpl;

class TransactionImpl : public Transaction {
public:
  TransactionImpl(std::string message_id, std::string transaction_id, std::string endpoint,
                  const std::shared_ptr<ProducerImpl>& producer)
      : message_id_(std::move(message_id)), transaction_id_(std::move(transaction_id)), endpoint_(std::move(endpoint)),
        producer_(producer) {}

  ~TransactionImpl() override = default;

  bool commit() override;

  bool rollback() override;

private:
  std::string message_id_;
  std::string transaction_id_;
  std::string endpoint_;
  std::weak_ptr<ProducerImpl> producer_;
};

ROCKETMQ_NAMESPACE_END