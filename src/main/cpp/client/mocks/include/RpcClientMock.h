#pragma once

#include "InvocationContext.h"
#include "RpcClient.h"
#include "gmock/gmock.h"
#include <iostream>

ROCKETMQ_NAMESPACE_BEGIN

class RpcClientMock : public RpcClient {
public:
  RpcClientMock();

  ~RpcClientMock() override { std::cout << "~RpcClientMock()" << std::endl; }

  MOCK_METHOD(void, asyncQueryRoute, (const QueryRouteRequest&, InvocationContext<QueryRouteResponse>*), (override));

  MOCK_METHOD(void, asyncSend, (const SendMessageRequest&, InvocationContext<SendMessageResponse>*), (override));

  MOCK_METHOD(void, asyncQueryAssignment, (const QueryAssignmentRequest&, InvocationContext<QueryAssignmentResponse>*),
              (override));

  MOCK_METHOD(std::shared_ptr<CompletionQueue>&, completionQueue, (), (override));

  MOCK_METHOD(void, asyncReceive, (const ReceiveMessageRequest&, InvocationContext<ReceiveMessageResponse>*),
              (override));

  MOCK_METHOD(void, asyncAck, (const AckMessageRequest&, InvocationContext<AckMessageResponse>*), (override));

  MOCK_METHOD(void, asyncNack, (const NackMessageRequest&, InvocationContext<NackMessageResponse>*), (override));

  MOCK_METHOD(void, asyncHeartbeat, (const HeartbeatRequest&, InvocationContext<HeartbeatResponse>*), (override));

  MOCK_METHOD(void, asyncHealthCheck, (const HealthCheckRequest&, InvocationContext<HealthCheckResponse>*), (override));

  MOCK_METHOD(void, asyncEndTransaction, (const EndTransactionRequest&, InvocationContext<EndTransactionResponse>*),
              (override));

  MOCK_METHOD(void, asyncMultiplexingCall, (const MultiplexingRequest&, InvocationContext<MultiplexingResponse>*),
              (override));

  MOCK_METHOD(void, asyncQueryOffset, (const QueryOffsetRequest&, InvocationContext<QueryOffsetResponse>*), (override));

  MOCK_METHOD(void, asyncPull, (const PullMessageRequest&, InvocationContext<PullMessageResponse>*), (override));

  MOCK_METHOD(void, asyncForwardMessageToDeadLetterQueue,
              (const ForwardMessageToDeadLetterQueueRequest&, InvocationContext<ForwardMessageToDeadLetterQueueResponse>*),
              (override));

  MOCK_METHOD(bool, needHeartbeat, (), (override));

  MOCK_METHOD(void, needHeartbeat, (bool), (override));

  MOCK_METHOD(bool, ok, (), (const override));

protected:
  std::shared_ptr<grpc::CompletionQueue> completion_queue_;
};

ROCKETMQ_NAMESPACE_END