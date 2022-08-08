#pragma once

#include "ONSFactoryProperty.h"

ONS_NAMESPACE_BEGIN

class ONSEnvironmentVariableFactoryProperty : public ONSFactoryProperty {
public:
  ONSEnvironmentVariableFactoryProperty();

  ~ONSEnvironmentVariableFactoryProperty() override = default;

private:
  void parseEnvironmentVariables();
};

ONS_NAMESPACE_END