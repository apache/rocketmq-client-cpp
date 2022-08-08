#include <cstdlib>
#include <iostream>

#include "ons/ONSFactory.h"
#include "ons/ONSFactoryProperty.h"

using namespace ons;

int main(int argc, char* argv[]) {
  ONSFactoryProperty factory_property;
  auto order_producer = ONSFactory::getInstance()->createOrderProducer(factory_property);
  order_producer->start();

  Message message("cpp_sdk_standard", "TagA", "Sample Body");
  SendResultONS send_result = order_producer->send(message, "message-group-0");

  std::cout << send_result.getMessageId() << std::endl;

  order_producer->shutdown();

  return EXIT_SUCCESS;
}