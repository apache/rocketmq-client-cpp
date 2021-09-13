#pragma once

#include "apache/rocketmq/v1/service.pb.h"
#include "grpcpp/grpcpp.h"

#include "InvocationContext.h"
#include "OrphanTransactionCallback.h"

#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"
#include "apache/rocketmq/v1/definition.grpc.pb.h"
#include "apache/rocketmq/v1/service.grpc.pb.h"
#include <chrono>
#include <iostream>
#include <memory>
#include <string>

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
using MultiplexingRequest = rmq::MultiplexingRequest;
using MultiplexingResponse = rmq::MultiplexingResponse;
using GenericPollingRequest = rmq::GenericPollingRequest;
using GenericPollingResponse = rmq::GenericPollingRequest;
using PrintThreadStackRequest = rmq::PrintThreadStackRequest;
using PrintThreadStackResponse = rmq::PrintThreadStackResponse;
using VerifyMessageConsumptionRequest = rmq::VerifyMessageConsumptionRequest;
using VerifyMessageConsumptionResponse = rmq::VerifyMessageConsumptionResponse;
using ResolveOrphanedTransactionRequest = rmq::ResolveOrphanedTransactionRequest;
using QueryOffsetRequest = rmq::QueryOffsetRequest;
using QueryOffsetResponse = rmq::QueryOffsetResponse;
using PullMessageRequest = rmq::PullMessageRequest;
using PullMessageResponse = rmq::PullMessageResponse;
using ForwardMessageToDeadLetterQueueRequest = rmq::ForwardMessageToDeadLetterQueueRequest;
using ForwardMessageToDeadLetterQueueResponse = rmq::ForwardMessageToDeadLetterQueueResponse;
using NotifyClientTerminationRequest = rmq::NotifyClientTerminationRequest;
using NotifyClientTerminationResponse = rmq::NotifyClientTerminationResponse;

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

  virtual void asyncMultiplexingCall(const MultiplexingRequest& request,
                                     InvocationContext<MultiplexingResponse>* invocation_context) = 0;

  virtual void asyncQueryOffset(const QueryOffsetRequest& request,
                                InvocationContext<QueryOffsetResponse>* invocation_context) = 0;

  virtual void asyncPull(const PullMessageRequest& request,
                         InvocationContext<PullMessageResponse>* invocation_context) = 0;

  virtual void asyncForwardMessageToDeadLetterQueue(
      const ForwardMessageToDeadLetterQueueRequest& request,
      InvocationContext<ForwardMessageToDeadLetterQueueResponse>* invocation_context) = 0;

  virtual grpc::Status notifyClientTermination(grpc::ClientContext* context,
                                               const NotifyClientTerminationRequest& request,
                                               NotifyClientTerminationResponse* response) = 0;

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