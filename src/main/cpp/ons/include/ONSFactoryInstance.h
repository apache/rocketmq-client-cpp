#pragma once

#include <memory>

#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"

#include "ons/ONSFactoryAPI.h"

ONS_NAMESPACE_BEGIN

class ONSFactoryInstance : public ONSFactoryAPI {
public:
  ONSFactoryInstance() = default;

  ~ONSFactoryInstance() override = default;

  Producer* createProducer(ONSFactoryProperty& factory_properties) override LOCKS_EXCLUDED(producer_table_mtx_);

  OrderProducer* createOrderProducer(ONSFactoryProperty& factory_properties) override
      LOCKS_EXCLUDED(order_producer_table_mtx_);

  OrderConsumer* createOrderConsumer(ONSFactoryProperty& factory_properties) override
      LOCKS_EXCLUDED(order_consumer_table_mtx_);

  TransactionProducer* createTransactionProducer(ONSFactoryProperty& factory_properties,
                                                 LocalTransactionChecker* checker) override
      LOCKS_EXCLUDED(transaction_producer_table_mtx_);

  PullConsumer* createPullConsumer(ONSFactoryProperty& factory_properties) override;

  PushConsumer* createPushConsumer(ONSFactoryProperty& factory_properties) override LOCKS_EXCLUDED(consumer_table_mtx_);

private:
  friend ONSFactoryAPI* instance();

  std::vector<std::shared_ptr<Producer>> producer_table_ GUARDED_BY(producer_table_mtx_);
  absl::Mutex producer_table_mtx_;

  std::vector<std::shared_ptr<OrderProducer>> order_producer_table_ GUARDED_BY(order_producer_table_mtx_);
  absl::Mutex order_producer_table_mtx_;

  std::vector<std::shared_ptr<TransactionProducer>>
      transaction_producer_table_ GUARDED_BY(transaction_producer_table_mtx_);
  absl::Mutex transaction_producer_table_mtx_;

  std::vector<std::shared_ptr<PushConsumer>> consumer_table_ GUARDED_BY(consumer_table_mtx_);
  absl::Mutex consumer_table_mtx_;

  std::vector<std::shared_ptr<OrderConsumer>> order_consumer_table_ GUARDED_BY(order_consumer_table_mtx_);
  absl::Mutex order_consumer_table_mtx_;
};

ONS_NAMESPACE_END