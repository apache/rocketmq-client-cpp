#pragma once

#include <utility>
#include <memory>

#include "Credentials.h"

ROCKETMQ_NAMESPACE_BEGIN

class CredentialsProvider {
public:
  virtual ~CredentialsProvider() = default;

  virtual Credentials getCredentials() = 0;

  virtual std::chrono::system_clock::duration refreshInterval() = 0;
};

using CredentialsProviderPtr = std::shared_ptr<CredentialsProvider>;

class StaticCredentialsProvider : public CredentialsProvider {
public:
  StaticCredentialsProvider(std::string access_key, std::string access_secret);

  ~StaticCredentialsProvider() override = default;

  Credentials getCredentials() override;

  std::chrono::system_clock::duration refreshInterval() override;

private:
  std::string access_key_;
  std::string access_secret_;
};

/**
 * Read credentials from environment variables ROCKETMQ_ACCESS_KEY, ROCKETMQ_ACCESS_SECRET
 */
class EnvironmentVariablesCredentialsProvider : public CredentialsProvider {
public:
  EnvironmentVariablesCredentialsProvider();

  ~EnvironmentVariablesCredentialsProvider() override = default;

  Credentials getCredentials() override;

  std::chrono::system_clock::duration refreshInterval() override { return std::chrono::system_clock::duration::zero(); }

private:
  std::string access_key_;
  std::string access_secret_;

  static const char* ENVIRONMENT_ACCESS_KEY;
  static const char* ENVIRONMENT_ACCESS_SECRET;
};

/**
 * Read credentials from configuration file: ~/rocketmq/credentials. By default, the client library would refresh every
 * 10 seconds.
 */
class ConfigFileCredentialsProvider : public CredentialsProvider {
public:
  ConfigFileCredentialsProvider();

  Credentials getCredentials() override;

  std::chrono::system_clock::duration refreshInterval() override;

  /**
   * For test purpose only.
   * @return
   */
  static const char* credentialFile() {
    return CREDENTIAL_FILE_;
  }
private:
  std::chrono::system_clock::duration refresh_interval_{std::chrono::seconds(10)};
  std::string access_key_;
  std::string access_secret_;
  static const char* CREDENTIAL_FILE_;
  static const char* ACCESS_KEY_FIELD_NAME;
  static const char* ACCESS_SECRET_FIELD_NAME;
};

ROCKETMQ_NAMESPACE_END