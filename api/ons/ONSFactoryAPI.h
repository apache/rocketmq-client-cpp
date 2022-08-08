#pragma once

#include "LocalTransactionChecker.h"
#include "MessageModel.h"
#include "ONSChannel.h"
#include "ONSClient.h"
#include "ONSClientException.h"
#include "ONSFactoryProperty.h"
#include "OrderConsumer.h"
#include "OrderProducer.h"
#include "Producer.h"
#include "PullConsumer.h"
#include "PushConsumer.h"
#include "TransactionProducer.h"

ONS_NAMESPACE_BEGIN

class ONSCLIENT_API ONSFactoryAPI {
public:
  virtual ~ONSFactoryAPI() = default;

  virtual Producer* createProducer(ONSFactoryProperty& factory_properties) = 0;

  virtual OrderProducer* createOrderProducer(ONSFactoryProperty& factory_properties) = 0;

  virtual OrderConsumer* createOrderConsumer(ONSFactoryProperty& factory_properties) = 0;

  virtual TransactionProducer* createTransactionProducer(ONSFactoryProperty& factory_properties,
                                                         LocalTransactionChecker* checker) = 0;

  virtual PullConsumer* createPullConsumer(ONSFactoryProperty& factory_properties) = 0;

  virtual PushConsumer* createPushConsumer(ONSFactoryProperty& factory_properties) = 0;
};

ONS_NAMESPACE_END