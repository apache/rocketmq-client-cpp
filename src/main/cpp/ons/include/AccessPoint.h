#pragma once

#include <string>

#include "absl/strings/string_view.h"

#include "rocketmq/Logger.h"
#include "rocketmq/RocketMQ.h"

#include "spdlog/spdlog.h"

#include "ons/ONSClient.h"

ONS_NAMESPACE_BEGIN

class AccessPoint {
public:
  explicit AccessPoint(absl::string_view access_point) : access_point_(access_point.data(), access_point.length()) {
    SPDLOG_INFO("Resource namespace={}, name-server-address={}", resourceNamespace(), nameServerAddress());
  }

  operator bool() const;

  std::string resourceNamespace() const;

  std::string nameServerAddress() const;

private:
  std::string access_point_;

  static const char* SCHEMA;

  static const char* RESOURCE_NAMESPACE_PREFIX;

  static const char* PREFIX;
};

ONS_NAMESPACE_END