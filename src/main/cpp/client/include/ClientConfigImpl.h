#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "absl/time/time.h"

#include "ClientConfig.h"

ROCKETMQ_NAMESPACE_BEGIN

class ClientConfigImpl : virtual public ClientConfig {
public:
  explicit ClientConfigImpl(absl::string_view group_name);

  ~ClientConfigImpl() override = default;

  const std::string &resourceNamespace() const override {
    return resource_namespace_;
  }

  void resourceNamespace(absl::string_view resource_namespace) {
    resource_namespace_ =
        std::string(resource_namespace.data(), resource_namespace.length());
  }

  std::string clientId() const override;

  const std::string &getInstanceName() const;

  void setInstanceName(std::string instance_name);

  const std::string &getGroupName() const override;
  void setGroupName(std::string group_name);

  const std::string &getUnitName() const { return unit_name_; }
  void setUnitName(std::string unit_name) { unit_name_ = std::move(unit_name); }

  absl::Duration getIoTimeout() const override;
  void setIoTimeout(absl::Duration timeout);

  absl::Duration getLongPollingTimeout() const override {
    return long_polling_timeout_;
  }

  void setLongPollingTimeout(absl::Duration timeout) {
    long_polling_timeout_ = timeout;
  }

  bool isTracingEnabled() { return enable_tracing_.load(); }
  void enableTracing(bool enabled) { enable_tracing_.store(enabled); }

  CredentialsProviderPtr credentialsProvider() override;
  void setCredentialsProvider(CredentialsProviderPtr credentials_provider);

  void serviceName(std::string service_name) {
    service_name_ = std::move(service_name);
  }
  const std::string &serviceName() const override { return service_name_; }

  void region(std::string region) { region_ = std::move(region); }
  const std::string &region() const override { return region_; }

  void tenantId(std::string tenant_id) { tenant_id_ = std::move(tenant_id); }
  const std::string &tenantId() const override { return tenant_id_; }

  static const char *CLIENT_VERSION;

protected:
  /**
   * Name of the service.
   */
  std::string service_name_{"ONS"};

  /**
   * Region of the service to connect to.
   */
  std::string region_;

  /**
   * RocketMQ instance namespace, in which topic, consumer group and any other
   * abstract resources remain unique.
   */
  std::string resource_namespace_;

  /**
   * Tenant identifier.
   */
  std::string tenant_id_;

  std::string instance_name_;

  std::string group_name_;

  std::string unit_name_;

  CredentialsProviderPtr credentials_provider_;

  absl::Duration io_timeout_;

  absl::Duration long_polling_timeout_;

  std::atomic<bool> enable_tracing_{false};
};

ROCKETMQ_NAMESPACE_END