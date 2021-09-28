#include "InvocationContext.h"
#include "RpcClient.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(InvocationContextTest, testOnCompletion_OK) {
  bool cb_invoked = false;
  auto callback = [&](const InvocationContext<SendMessageResponse>* invocation_context) {
    cb_invoked = true;
    throw 1;
  };

  auto invocation_context = new InvocationContext<SendMessageResponse>();
  invocation_context->status = grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED, "Mock deadline exceeded");
  invocation_context->callback = callback;
  invocation_context->onCompletion(true);
  EXPECT_TRUE(cb_invoked);
}

ROCKETMQ_NAMESPACE_END