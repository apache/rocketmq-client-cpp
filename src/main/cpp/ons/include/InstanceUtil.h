#pragma once

#include "ONSUtil.h"
#include <string>

ONS_NAMESPACE_BEGIN

const std::string INSTANCE_PREFIX = "MQ_INST_";
static const std::string ENDPOINT_PREFIX = "http://";
const std::string INSTANCE_REGEX = INSTANCE_PREFIX + "\\d+_\\w{8}";
const std::string RETRY_INSTANCE_PREFIX = "%RETRY%" + INSTANCE_PREFIX;
const int INSTANCE_PREFIX_LENGTH = INSTANCE_PREFIX.length();
const std::string MISC_HEAD = "http://MQ_INST_";

class InstanceUtil {
public:
  static bool validateInstanceEndpoint(const std::string& endpoint);

  static std::string parseInstanceIdFromEndpoint(const std::string& endpoint);

  static bool validateNameSrvAddr(const std::string& nameServerAddr);
};

ONS_NAMESPACE_END