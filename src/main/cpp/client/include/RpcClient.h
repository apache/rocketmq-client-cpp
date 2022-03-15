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

#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"
#include "apache/rocketmq/v1/definition.grpc.pb.h"
#include "apache/rocketmq/v1/service.grpc.pb.h"
#include "apache/rocketmq/v1/service.pb.h"
#include "grpcpp/grpcpp.h"

#include "InvocationContext.h"
#include "OrphanTransactionCallback.h"

ROCKETMQ_NAMESPACE_BEGIN

namespace rmq = apache::rocketmq::v1;

using Channel = grpc::Channel;
using Status = grpc::Status;
using CompletionQueue = grpc::CompletionQueue;
using QueryRouteRequest = rmq::QueryRouteRequest;
using QueryRouteResponse = rmq::QueryRouteResponse;
using SendMessageRequest = rmq::SendMessageRequest;
using SendMessageResponse = rmq::SendMessageResponse;
using QueryAssignmentRequest = rmq::QueryAssignmentRequest;
using QueryAssignmentResponse = rmq::QueryAssignmentResponse;
using ReceiveMessageRequest = rmq::ReceiveMessageRequest;
using ReceiveMessageResponse = rmq::ReceiveMessageResponse;
using AckMessageRequest = rmq::AckMessageRequest;
using AckMessageResponse = rmq::AckMessageResponse;
using NackMessageRequest = rmq::NackMessageRequest;
using NackMessageResponse = rmq::NackMessageResponse;
using HeartbeatRequest = rmq::HeartbeatRequest;
using HeartbeatResponse = rmq::HeartbeatResponse;
using HealthCheckRequest = rmq::HealthCheckRequest;
using HealthCheckResponse = rmq::HealthCheckResponse;
using EndTransactionRequest = rmq::EndTransactionRequest;
using EndTransactionResponse = rmq::EndTransactionResponse;
using QueryOffsetRequest = rmq::QueryOffsetRequest;
using QueryOffsetResponse = rmq::QueryOffsetResponse;
using PullMessageRequest = rmq::PullMessageRequest;
using PullMessageResponse = rmq::PullMessageResponse;
using PollCommandRequest = rmq::PollCommandRequest;
using PollCommandResponse = rmq::PollCommandResponse;
using ReportThreadStackTraceRequest = rmq::ReportThreadStackTraceRequest;
using ReportThreadStackTraceResponse = rmq::ReportThreadStackTraceResponse;
using ReportMessageConsumptionResultRequest = rmq::ReportMessageConsumptionResultRequest;
using ReportMessageConsumptionResultResponse = rmq::ReportMessageConsumptionResultResponse;
using ForwardMessageToDeadLetterQueueRequest = rmq::ForwardMessageToDeadLetterQueueRequest;
using ForwardMessageToDeadLetterQueueResponse = rmq::ForwardMessageToDeadLetterQueueResponse;
using NotifyClientTerminationRequest = rmq::NotifyClientTerminationRequest;
using NotifyClientTerminationResponse = rmq::NotifyClientTerminationResponse;
using ChangeInvisibleDurationRequest = rmq::ChangeInvisibleDurationRequest;
using ChangeInvisibleDurationResponse = rmq::ChangeInvisibleDurationResponse;

/**
 * @brief A RpcClient represents a session between client and a remote broker.
 *
 */
class RpcClient {
public:
  RpcClient() = default;

  virtual ~RpcClient() = default;

  virtual void asyncQueryRoute(const QueryRouteRequest& request,
                               InvocationContext<QueryRouteResponse>* invocation_context) = 0;

  virtual void asyncSend(const SendMessageRequest& request,
                         InvocationContext<SendMessageResponse>* invocation_context) = 0;

  virtual void asyncQueryAssignment(const QueryAssignmentRequest& request,
                                    InvocationContext<QueryAssignmentResponse>* invocation_context) = 0;

  virtual std::shared_ptr<CompletionQueue>& completionQueue() = 0;

  virtual void asyncReceive(const ReceiveMessageRequest& request,
                            InvocationContext<ReceiveMessageResponse>* invocation_context) = 0;

  virtual void asyncAck(const AckMessageRequest& request,
                        InvocationContext<AckMessageResponse>* invocation_context) = 0;

  virtual void asyncNack(const NackMessageRequest& request,
                         InvocationContext<NackMessageResponse>* invocation_context) = 0;

  virtual void asyncHeartbeat(const HeartbeatRequest& request,
                              InvocationContext<HeartbeatResponse>* invocation_context) = 0;

  virtual void asyncHealthCheck(const HealthCheckRequest& request,
                                InvocationContext<HealthCheckResponse>* invocation_context) = 0;

  virtual void asyncEndTransaction(const EndTransactionRequest& request,
                                   InvocationContext<EndTransactionResponse>* invocation_context) = 0;

  virtual void asyncPollCommand(const PollCommandRequest& request,
                                InvocationContext<PollCommandResponse>* invocation_context) = 0;

  virtual void asyncQueryOffset(const QueryOffsetRequest& request,
                                InvocationContext<QueryOffsetResponse>* invocation_context) = 0;

  virtual void asyncPull(const PullMessageRequest& request,
                         InvocationContext<PullMessageResponse>* invocation_context) = 0;

  virtual void asyncForwardMessageToDeadLetterQueue(
      const ForwardMessageToDeadLetterQueueRequest& request,
      InvocationContext<ForwardMessageToDeadLetterQueueResponse>* invocation_context) = 0;

  virtual grpc::Status reportThreadStackTrace(grpc::ClientContext* context,
                                              const ReportThreadStackTraceRequest& request,
                                              ReportThreadStackTraceResponse* response) = 0;

  virtual grpc::Status reportMessageConsumptionResult(grpc::ClientContext* context,
                                                      const ReportMessageConsumptionResultRequest& request,
                                                      ReportMessageConsumptionResultResponse* response) = 0;

  virtual grpc::Status notifyClientTermination(grpc::ClientContext* context,
                                               const NotifyClientTerminationRequest& request,
                                               NotifyClientTerminationResponse* response) = 0;

  virtual void asyncChangeInvisibleDuration(const ChangeInvisibleDurationRequest&,
                                            InvocationContext<ChangeInvisibleDurationResponse>*) = 0;

  /**
   * Indicate if heartbeat is required.
   * @return true if periodic heartbeat is required; false otherwise.
   */
  virtual bool needHeartbeat() = 0;

  virtual void needHeartbeat(bool need_heartbeat) = 0;

  /**
   * Indicate if current client connection state is OK or recoverable.
   *
   * @return true if underlying connection is OK or recoverable; false otherwise.
   */
  virtual bool ok() const = 0;
};

ROCKETMQ_NAMESPACE_END