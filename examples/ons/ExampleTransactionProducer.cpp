/*
* Licensed to the Apache Software Foundation (ASF) under one or more
* contributor license agreements.  See the NOTICE file distributed with
* this work for additional information regarding copyright ownership.
* The ASF licenses this file to You under the Apache License, Version 2.0
* (the "License"); you may not use this file except in compliance with
* the License.  You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "ons/TransactionProducer.h"

#include <cstdlib>
#include <iostream>

#include "absl/memory/memory.h"

#include "ons/LocalTransactionChecker.h"
#include "ons/ONSFactory.h"
#include "ons/TransactionStatus.h"

using namespace ons;

namespace ons {

class ExampleLocalTransactionChecker : public LocalTransactionChecker {
public:
  TransactionStatus check(const Message& msg) noexcept override { return TransactionStatus::CommitTransaction; }
};

class ExampleLocalTransactionExecutor : public LocalTransactionExecuter {
public:
  TransactionStatus execute(const Message& msg) noexcept override { return TransactionStatus::CommitTransaction; }
};

} // namespace ons

int main(int argc, char* argv[]) {
  ONSFactoryProperty factory_property;

  auto checker = absl::make_unique<ExampleLocalTransactionChecker>();
  auto executor = absl::make_unique<ExampleLocalTransactionExecutor>();

  auto transaction_producer = ONSFactory::getInstance()->createTransactionProducer(factory_property, checker.get());
  transaction_producer->start();

  Message message("cpp_sdk_standard", "TagA", "Sample Body");
  transaction_producer->send(message, executor.get());

  transaction_producer->shutdown();

  return EXIT_SUCCESS;
}