#pragma once

#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"
#include "rocketmq/CredentialsProvider.h"
#include "HttpClient.h"

ROCKETMQ_NAMESPACE_BEGIN

class StsCredentialsProviderImpl : public CredentialsProvider {
public:
  explicit StsCredentialsProviderImpl(std::string ram_role_name);

  ~StsCredentialsProviderImpl() override;

  Credentials getCredentials() override;

  /**
   * @brief Expose http_client for test purpose only.
   * 
   * @param http_client
   */
  void setHttpClient(std::unique_ptr<HttpClient> http_client) {
    http_client_ = std::move(http_client);
  }

private:
  static const char* RAM_ROLE_HOST;
  static const char* RAM_ROLE_URL_PREFIX;
  static const char* FIELD_ACCESS_KEY;
  static const char* FIELD_ACCESS_SECRET;
  static const char* FIELD_SECURITY_TOKEN;
  static const char* FIELD_EXPIRATION;
  static const char* EXPIRATION_DATE_TIME_FORMAT;
  
  std::string ram_role_name_;

  std::string access_key_ GUARDED_BY(mtx_);
  std::string access_secret_ GUARDED_BY(mtx_);
  std::string security_token_ GUARDED_BY(mtx_);
  std::chrono::system_clock::time_point expiration_;

  absl::Mutex mtx_;
  void refresh() LOCKS_EXCLUDED(mtx_);

  std::unique_ptr<HttpClient> http_client_;
};

ROCKETMQ_NAMESPACE_END