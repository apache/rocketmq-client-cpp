#pragma once

#include "rocketmq/RocketMQ.h"
#include <chrono>
#include <string>
#include <utility>

ROCKETMQ_NAMESPACE_BEGIN

class Credentials {
public:
  Credentials() : expiration_instant_(std::chrono::system_clock::time_point::max()) {}

  Credentials(std::string access_key, std::string access_secret)
      : access_key_(std::move(access_key)), access_secret_(std::move(access_secret)),
        expiration_instant_(std::chrono::system_clock::time_point::max()) {}

  Credentials(std::string access_key, std::string access_secret, std::string session_token,
              std::chrono::system_clock::time_point expiration)
      : access_key_(std::move(access_key)), access_secret_(std::move(access_secret)),
        session_token_(std::move(session_token)), expiration_instant_(expiration) {}

  bool operator==(const Credentials& rhs) const {
    return access_key_ == rhs.access_key_ && access_secret_ == rhs.access_secret_ &&
           session_token_ == rhs.session_token_ && expiration_instant_ == rhs.expiration_instant_;
  }

  bool operator!=(const Credentials& rhs) const { return !(*this == rhs); }

  bool empty() const { return access_key_.empty() || access_secret_.empty(); }

  bool expired() const { return std::chrono::system_clock::now() > expiration_instant_; }

  const std::string& accessKey() const { return access_key_; }

  const std::string& accessSecret() const { return access_secret_; }

  const std::string& sessionToken() const { return session_token_; }

  std::chrono::system_clock::time_point expirationInstant() const { return expiration_instant_; }

private:
  std::string access_key_;
  std::string access_secret_;
  std::string session_token_;
  std::chrono::system_clock::time_point expiration_instant_;
};

ROCKETMQ_NAMESPACE_END