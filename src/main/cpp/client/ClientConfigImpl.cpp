#include "ClientConfigImpl.h"
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

const char* ClientConfigImpl::CLIENT_VERSION = CLIENT_VERSION_MAJOR "." CLIENT_VERSION_MINOR "." CLIENT_VERSION_PATCH;

ClientConfigImpl::ClientConfigImpl() : ClientConfigImpl(std::string()) {}

ClientConfigImpl::ClientConfigImpl(std::string group_name)
    : group_name_(std::move(group_name)), io_timeout_(absl::Seconds(3)), long_polling_timeout_(absl::Seconds(30)) {
  instance_name_ = "DEFAULT";
}

std::string ClientConfigImpl::clientId() const {
  std::stringstream ss;
  ss << UtilAll::hostname();
  ss << "@";
  std::string processID = std::to_string(getpid());
  ss << (instance_name_ == "DEFAULT" ? processID : instance_name_);
  return ss.str();
}

const std::string& ClientConfigImpl::getInstanceName() const { return instance_name_; }

void ClientConfigImpl::setInstanceName(std::string instance_name) { instance_name_ = std::move(instance_name); }

const std::string& ClientConfigImpl::getGroupName() const { return group_name_; }

void ClientConfigImpl::setGroupName(std::string group_name) { group_name_ = std::move(group_name); }

void ClientConfigImpl::setIoTimeout(absl::Duration timeout) { io_timeout_ = timeout; }

void ClientConfigImpl::setCredentialsProvider(CredentialsProviderPtr credentials_provider) {
  credentials_provider_ = std::move(credentials_provider);
}

CredentialsProviderPtr ClientConfigImpl::credentialsProvider() { return credentials_provider_; }

absl::Duration ClientConfigImpl::getIoTimeout() const { return io_timeout_; }

ROCKETMQ_NAMESPACE_END