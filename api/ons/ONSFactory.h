#pragma once

#include "ONSFactoryAPI.h"

ONS_NAMESPACE_BEGIN

class ONSCLIENT_API ONSFactory {
public:
  ONSFactory() = delete;

  virtual ~ONSFactory() = default;

  static ONSFactoryAPI* getInstance();
};

ONS_NAMESPACE_END