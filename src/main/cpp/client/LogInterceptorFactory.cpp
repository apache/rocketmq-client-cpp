#include "LogInterceptorFactory.h"
#include "LogInterceptor.h"

ROCKETMQ_NAMESPACE_BEGIN

grpc::experimental::Interceptor*
LogInterceptorFactory::CreateClientInterceptor(grpc::experimental::ClientRpcInfo* info) {
  return new LogInterceptor(info);
}

ROCKETMQ_NAMESPACE_END