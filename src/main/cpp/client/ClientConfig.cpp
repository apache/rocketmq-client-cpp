#include "ClientConfig.h"
#include "UtilAll.h"
#include <sstream>
#include <unistd.h>

ROCKETMQ_NAMESPACE_BEGIN

#ifndef CLIENT_VERSION_MAJOR
#define CLIENT_VERSION_MAJOR "1"
#endif

#ifndef CLIENT_VERSION_MINOR
#define CLIENT_VERSION_MINOR "0"
#endif

#ifndef CLIENT_VERSION_PATCH
#define CLIENT_VERSION_PATCH "0"
#endif

const char* ClientConfig::CLIENT_VERSION = CLIENT_VERSION_MAJOR "." CLIENT_VERSION_MINOR "." CLIENT_VERSION_PATCH;

ClientConfig::ClientConfig() : ClientConfig(std::string()) {}

ClientConfig::ClientConfig(std::string group_name)
    : group_name_(std::move(group_name)), io_timeout_(absl::Seconds(3)), long_polling_timeout_(absl::Seconds(30)) {
  instance_name_ = "DEFAULT";
}

std::string ClientConfig::clientId() const {
  std::stringstream ss;
  ss << UtilAll::hostname();
  ss << "@";
  std::string processID = std::to_string(getpid());
  ss << (instance_name_ == "DEFAULT" ? processID : instance_name_);
  return ss.str();
}

const std::string& ClientConfig::getInstanceName() const { return instance_name_; }

void ClientConfig::setInstanceName(std::string instance_name) { instance_name_ = std::move(instance_name); }

const std::string& ClientConfig::getGroupName() const { return group_name_; }

void ClientConfig::setGroupName(std::string group_name) { group_name_ = std::move(group_name); }

void ClientConfig::setIoTimeout(absl::Duration timeout) { io_timeout_ = timeout; }

void ClientConfig::setCredentialsProvider(CredentialsProviderPtr credentials_provider) {
  credentials_provider_ = std::move(credentials_provider);
}

CredentialsProviderPtr ClientConfig::credentialsProvider() { return credentials_provider_; }

absl::Duration ClientConfig::getIoTimeout() const { return io_timeout_; }

ROCKETMQ_NAMESPACE_END