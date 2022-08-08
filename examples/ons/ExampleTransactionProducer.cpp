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