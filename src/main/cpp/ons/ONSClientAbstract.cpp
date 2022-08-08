#include "ONSClientAbstract.h"

#include "rocketmq/Logger.h"
#include "spdlog/spdlog.h"

ONS_NAMESPACE_BEGIN

// TODO: set AccessKey and SecretKey here.
ONSClientAbstract::ONSClientAbstract(const ONSFactoryProperty& factory_property)
    : factory_property_(factory_property), access_point_(factory_property.getNameSrvAddr()) {}

void ONSClientAbstract::start() {}

void ONSClientAbstract::shutdown() {}

// TODO: not yet implemented.
std::string ONSClientAbstract::buildInstanceName() { return "DefaultInstanceName"; }

ONS_NAMESPACE_END