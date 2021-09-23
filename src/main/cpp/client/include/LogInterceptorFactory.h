#pragma once

#include "grpcpp/impl/codegen/client_interceptor.h"

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class LogInterceptorFactory : public grpc::experimental::ClientInterceptorFactoryInterface {
public:
  grpc::experimental::Interceptor* CreateClientInterceptor(grpc::experimental::ClientRpcInfo* info) override;
};

ROCKETMQ_NAMESPACE_END