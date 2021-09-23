#include "grpcpp/impl/codegen/client_interceptor.h"

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class LogInterceptor : public grpc::experimental::Interceptor {
public:
  explicit LogInterceptor(grpc::experimental::ClientRpcInfo* client_rpc_info) : client_rpc_info_(client_rpc_info) {}

  void Intercept(grpc::experimental::InterceptorBatchMethods* methods) override;

private:
  grpc::experimental::ClientRpcInfo* client_rpc_info_;
};

ROCKETMQ_NAMESPACE_END