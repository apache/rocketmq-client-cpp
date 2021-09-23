#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <utility>

#include "Credentials.h"

ROCKETMQ_NAMESPACE_BEGIN

class CredentialsProvider {
public:
  virtual ~CredentialsProvider() = default;

  virtual Credentials getCredentials() = 0;
};

using CredentialsProviderPtr = std::shared_ptr<CredentialsProvider>;

class StaticCredentialsProvider : public CredentialsProvider {
public:
  StaticCredentialsProvider(std::string access_key, std::string access_secret);

  ~StaticCredentialsProvider() override = default;

  Credentials getCredentials() override;

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

  static const char* ENVIRONMENT_ACCESS_KEY;
  static const char* ENVIRONMENT_ACCESS_SECRET;

private:
  std::string access_key_;
  std::string access_secret_;
};

/**
 * Read credentials from configuration file: ~/rocketmq/credentials. By default, the client library would refresh every
 * 10 seconds.
 */
class ConfigFileCredentialsProvider : public CredentialsProvider {
public:
  ConfigFileCredentialsProvider();

  ConfigFileCredentialsProvider(std::string config_file, std::chrono::milliseconds refresh_interval);

  Credentials getCredentials() override;

  /**
   * For test purpose only.
   * @return
   */
  static const char* credentialFile() { return CREDENTIAL_FILE_; }

private:
  std::chrono::system_clock::duration refresh_interval_{std::chrono::seconds(10)};
  std::string access_key_;
  std::string access_secret_;
  static const char* CREDENTIAL_FILE_;
  static const char* ACCESS_KEY_FIELD_NAME;
  static const char* ACCESS_SECRET_FIELD_NAME;
};

class StsCredentialsProviderImpl;

class StsCredentialsProvider : public CredentialsProvider {
public:
  explicit StsCredentialsProvider(std::string ram_role_name);

  Credentials getCredentials() override;

private:
  std::unique_ptr<StsCredentialsProviderImpl> impl_;
};

ROCKETMQ_NAMESPACE_END