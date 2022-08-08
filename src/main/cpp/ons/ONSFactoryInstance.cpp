#include <iostream>

#include "ONSFactoryInstance.h"

#include "ConsumerImpl.h"
#include "FAQ.h"
#include "ONSUtil.h"
#include "OrderConsumerImpl.h"
#include "OrderProducerImpl.h"
#include "ProducerImpl.h"
#include "TransactionProducerImpl.h"
#include "ons/ONSClientException.h"
#include "ons/ONSFactory.h"

ONS_NAMESPACE_BEGIN

Producer* ONSFactoryInstance::createProducer(ONSFactoryProperty& factory_properties) {
  if (ONSChannel::INNER == factory_properties.getOnsChannel()) {
    factory_properties.setOnsTraceSwitch(false);
    factory_properties.setFactoryProperty(ONSFactoryProperty::AccessKey, "DefaultKey");
    factory_properties.setFactoryProperty(ONSFactoryProperty::SecretKey, "DefaultKey");
    factory_properties.setFactoryProperty(ONSFactoryProperty::NAMESRV_ADDR, "LocalDefault");
  }

  if (!factory_properties) {
    ons::ONSClientException e(
        FAQ::errorMessage("Required configuration items are missing", FAQ::CLIENT_CHECK_MSG_EXCEPTION));
    throw e;
  }

  std::shared_ptr<ProducerImpl> producer = std::make_shared<ProducerImpl>(factory_properties);
  {
    absl::MutexLock guard(&producer_table_mtx_);
    producer_table_.push_back(producer);
  }
  return producer.get();
}

OrderProducer* ONSFactoryInstance::createOrderProducer(ONSFactoryProperty& factory_properties) {
  if (ONSChannel::INNER == factory_properties.getOnsChannel()) {
    factory_properties.setOnsTraceSwitch(false);
    factory_properties.setFactoryProperty(ONSFactoryProperty::AccessKey, "DefaultKey");
    factory_properties.setFactoryProperty(ONSFactoryProperty::SecretKey, "DefaultKey");
    factory_properties.setFactoryProperty(ONSFactoryProperty::NAMESRV_ADDR, "LocalDefault");
  }

  if (!factory_properties) {
    ons::ONSClientException e(
        FAQ::errorMessage("Required configuration items are missing", FAQ::CLIENT_CHECK_MSG_EXCEPTION));
    throw e;
  }

  std::shared_ptr<OrderProducerImpl> order_producer = std::make_shared<OrderProducerImpl>(factory_properties);
  {
    absl::MutexLock guard(&order_producer_table_mtx_);
    order_producer_table_.push_back(order_producer);
  }
  return order_producer.get();
}

OrderConsumer* ONSFactoryInstance::createOrderConsumer(ONSFactoryProperty& factory_properties) {
  if (ONSChannel::INNER == factory_properties.getOnsChannel()) {
    factory_properties.setOnsTraceSwitch(false);
    factory_properties.setFactoryProperty(ONSFactoryProperty::AccessKey, "DefaultKey");
    factory_properties.setFactoryProperty(ONSFactoryProperty::SecretKey, "DefaultKey");
    factory_properties.setFactoryProperty(ONSFactoryProperty::NAMESRV_ADDR, "LocalDefault");
  }

  if (!factory_properties) {
    ons::ONSClientException e(
        FAQ::errorMessage("Required configuration items are missing", FAQ::CLIENT_CHECK_MSG_EXCEPTION));
    throw e;
  }

  std::shared_ptr<OrderConsumerImpl> order_consumer = std::make_shared<OrderConsumerImpl>(factory_properties);
  {
    absl::MutexLock guard(&order_consumer_table_mtx_);
    order_consumer_table_.push_back(order_consumer);
  }
  return order_consumer.get();
}

TransactionProducer* ONSFactoryInstance::createTransactionProducer(ONSFactoryProperty& factory_properties,
                                                                   LocalTransactionChecker* checker) {
  if (ONSChannel::INNER == factory_properties.getOnsChannel()) {
    factory_properties.setOnsTraceSwitch(false);
    factory_properties.setFactoryProperty(ONSFactoryProperty::AccessKey, "DefaultKey");
    factory_properties.setFactoryProperty(ONSFactoryProperty::SecretKey, "DefaultKey");
    factory_properties.setFactoryProperty(ONSFactoryProperty::NAMESRV_ADDR, "LocalDefault");
  }

  if (!factory_properties) {
    ons::ONSClientException e(
        FAQ::errorMessage("Required configuration items are missing", FAQ::CLIENT_CHECK_MSG_EXCEPTION));
    throw e;
  }

  if (checker == nullptr) {
    std::string msg = "Transaction Checker cannot be NULL. Please check your ONS property set.";
    throw ONSClientException(msg);
  }
  std::shared_ptr<TransactionProducerImpl> transaction_producer =
      std::make_shared<TransactionProducerImpl>(factory_properties, checker);
  {
    absl::MutexLock guard(&transaction_producer_table_mtx_);
    transaction_producer_table_.push_back(transaction_producer);
  }
  return transaction_producer.get();
}

PullConsumer* ONSFactoryInstance::createPullConsumer(ONSFactoryProperty&) {
  throw ONSClientException("Pull is not supported for now.");
}

PushConsumer* ONSFactoryInstance::createPushConsumer(ONSFactoryProperty& factory_properties) {
  if (ONSChannel::INNER == factory_properties.getOnsChannel()) {
    factory_properties.setOnsTraceSwitch(false);
    factory_properties.setFactoryProperty(ONSFactoryProperty::AccessKey, "DefaultKey");
    factory_properties.setFactoryProperty(ONSFactoryProperty::SecretKey, "DefaultKey");
    factory_properties.setFactoryProperty(ONSFactoryProperty::NAMESRV_ADDR, "LocalDefault");
  }

  if (!factory_properties) {
    ons::ONSClientException e(
        FAQ::errorMessage("Required configuration items are missing", FAQ::CLIENT_CHECK_MSG_EXCEPTION));
    throw e;
  }

  std::shared_ptr<ConsumerImpl> consumer = std::make_shared<ConsumerImpl>(factory_properties);
  {
    absl::MutexLock guard(&consumer_table_mtx_);
    consumer_table_.push_back(consumer);
  }
  return consumer.get();
}

ONS_NAMESPACE_END