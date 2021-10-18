/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "InvocationContext.h"
#include "RpcClient.h"
#include "gmock/gmock.h"
#include <iostream>

ROCKETMQ_NAMESPACE_BEGIN

class RpcClientMock : public RpcClient {
public:
  RpcClientMock();

  ~RpcClientMock() override {
    std::cout << "~RpcClientMock()" << std::endl;
  }

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

  MOCK_METHOD(void, asyncQueryOffset, (const QueryOffsetRequest&, InvocationContext<QueryOffsetResponse>*), (override));

  MOCK_METHOD(void, asyncPull, (const PullMessageRequest&, InvocationContext<PullMessageResponse>*), (override));

  MOCK_METHOD(void, asyncForwardMessageToDeadLetterQueue,
              (const ForwardMessageToDeadLetterQueueRequest&,
               InvocationContext<ForwardMessageToDeadLetterQueueResponse>*),
              (override));

  MOCK_METHOD(void, asyncPollCommand, (const PollCommandRequest&, InvocationContext<PollCommandResponse>*), (override));

  MOCK_METHOD(grpc::Status, reportThreadStackTrace,
              (grpc::ClientContext*, const ReportThreadStackTraceRequest&, ReportThreadStackTraceResponse*),
              (override));

  MOCK_METHOD(grpc::Status, reportMessageConsumptionResult,
              (grpc::ClientContext*, const ReportMessageConsumptionResultRequest&,
               ReportMessageConsumptionResultResponse*),
              (override));

  MOCK_METHOD(grpc::Status, notifyClientTermination,
              (grpc::ClientContext*, const NotifyClientTerminationRequest&, NotifyClientTerminationResponse* response),
              (override));

  MOCK_METHOD(bool, needHeartbeat, (), (override));

  MOCK_METHOD(void, needHeartbeat, (bool), (override));

  MOCK_METHOD(bool, ok, (), (const override));

protected:
  std::shared_ptr<grpc::CompletionQueue> completion_queue_;
};

ROCKETMQ_NAMESPACE_END