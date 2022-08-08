#include "InstanceUtil.h"
#include "rocketmq/Logger.h"
#include "spdlog/spdlog.h"

ONS_NAMESPACE_BEGIN

bool InstanceUtil::validateInstanceEndpoint(const std::string& endpoint) {
  size_t position = endpoint.find(MISC_HEAD);
  if (position == 0) {
    position = endpoint.find('.');
    if (position != std::string::npos && position > MISC_HEAD.length()) {
      return true;
    }
  }
  return false;
}

std::string InstanceUtil::parseInstanceIdFromEndpoint(const std::string& endpoint) {
  std::string instanceId;
  if (!endpoint.empty()) {
    instanceId = endpoint.substr(ENDPOINT_PREFIX.length(), endpoint.find('.') - ENDPOINT_PREFIX.length());
  }
  SPDLOG_INFO("nameSpace is:{}, nameserver is:{}", instanceId.c_str(), endpoint.c_str());
  return instanceId;
}

bool InstanceUtil::validateNameSrvAddr(const std::string& name_server_address) {
  size_t position = name_server_address.find(ENDPOINT_PREFIX);
  if (position == 0 && name_server_address.length() > ENDPOINT_PREFIX.length()) {
    SPDLOG_INFO("validate true {}, nameserver is:{}", position, name_server_address.c_str());
    return true;
  }
  return false;
}

ONS_NAMESPACE_END