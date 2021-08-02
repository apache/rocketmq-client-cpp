#include "RpcClientMock.h"

using namespace testing;

ROCKETMQ_NAMESPACE_BEGIN

/**
 * TODO: Add commonly used mock operation for RpcClient
 */
RpcClientMock::RpcClientMock() : completion_queue_(std::make_shared<grpc::CompletionQueue>()) {
  ON_CALL(*this, completionQueue()).WillByDefault(ReturnRef(completion_queue_));
}

ROCKETMQ_NAMESPACE_END