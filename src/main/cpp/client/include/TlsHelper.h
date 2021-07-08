#pragma once

#include "rocketmq/RocketMQ.h"
#include <string>
#include <utility>

#include "grpcpp/security/tls_credentials_options.h"
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <openssl/x509.h>

#include "spdlog/spdlog.h"

ROCKETMQ_NAMESPACE_BEGIN

class TlsHelper {

public:
  static std::string sign(const std::string& access_secret, const std::string& content);

  static const char* CA;

  static const char* client_certificate_chain;

  static const char* client_private_key;
};

class TlsServerAuthorizationChecker : public grpc::experimental::TlsServerAuthorizationCheckInterface {
public:
  int Schedule(grpc::experimental::TlsServerAuthorizationCheckArg* arg) override {
    if (nullptr == arg) {
      return 0;
    }

    arg->set_success(1);
    arg->set_status(GRPC_STATUS_OK);
    return 0;
  }

  ~TlsServerAuthorizationChecker() override { SPDLOG_DEBUG("~TlsServerAuthorizationChecker() invoked"); }
};

ROCKETMQ_NAMESPACE_END