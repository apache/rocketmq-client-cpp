#include "ons/ONSFactory.h"

#include "ONSFactoryInstance.h"

ONS_NAMESPACE_BEGIN

ONSFactoryAPI* ONSFactory::getInstance() {
  static ONSFactoryInstance instance_;
  return &instance_;
}

ONS_NAMESPACE_END