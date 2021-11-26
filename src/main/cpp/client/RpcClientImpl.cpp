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
#include "RpcClientImpl.h"

#include <chrono>

#include "ClientConfigImpl.h"
#include "TlsHelper.h"
#include "absl/time/time.h"

using ClientContext = grpc::ClientContext;

ROCKETMQ_NAMESPACE_BEGIN

void RpcClientImpl::connect() {
}

void RpcClientImpl::asyncQueryRoute(const QueryRouteRequest& request,
                                    InvocationContext<QueryRouteResponse>* invocation_context) {
  invocation_context->response_reader =
      stub_->PrepareAsyncQueryRoute(&invocation_context->context, request, completion_queue_.get());
  invocation_context->response_reader->StartCall();
  invocation_context->response_reader->Finish(&invocation_context->response, &invocation_context->status,
                                              invocation_context);
}

void RpcClientImpl::asyncSend(const SendMessageRequest& request,
                              InvocationContext<SendMessageResponse>* invocation_context) {
  invocation_context->response_reader =
      stub_->PrepareAsyncSendMessage(&invocation_context->context, request, completion_queue_.get());
  invocation_context->response_reader->StartCall();
  invocation_context->response_reader->Finish(&invocation_context->response, &invocation_context->status,
                                              invocation_context);
}

void RpcClientImpl::asyncQueryAssignment(const QueryAssignmentRequest& request,
                                         InvocationContext<QueryAssignmentResponse>* invocation_context) {
  invocation_context->response_reader =
      stub_->PrepareAsyncQueryAssignment(&invocation_context->context, request, completion_queue_.get());
  invocation_context->response_reader->StartCall();
  invocation_context->response_reader->Finish(&invocation_context->response, &invocation_context->status,
                                              invocation_context);
}

std::shared_ptr<grpc::CompletionQueue>& rocketmq::RpcClientImpl::completionQueue() {
  return completion_queue_;
}

void RpcClientImpl::asyncReceive(const ReceiveMessageRequest& request,
                                 InvocationContext<ReceiveMessageResponse>* invocation_context) {
  invocation_context->response_reader =
      stub_->PrepareAsyncReceiveMessage(&invocation_context->context, request, completion_queue_.get());
  invocation_context->response_reader->StartCall();
  invocation_context->response_reader->Finish(&invocation_context->response, &invocation_context->status,
                                              invocation_context);
}

void RpcClientImpl::asyncAck(const AckMessageRequest& request,
                             InvocationContext<AckMessageResponse>* invocation_context) {
  assert(invocation_context);
  invocation_context->response_reader =
      stub_->PrepareAsyncAckMessage(&invocation_context->context, request, completion_queue_.get());
  invocation_context->response_reader->StartCall();
  invocation_context->response_reader->Finish(&invocation_context->response, &invocation_context->status,
                                              invocation_context);
}

void RpcClientImpl::asyncNack(const NackMessageRequest& request,
                              InvocationContext<NackMessageResponse>* invocation_context) {
  assert(invocation_context);
  invocation_context->response_reader =
      stub_->PrepareAsyncNackMessage(&invocation_context->context, request, completion_queue_.get());
  invocation_context->response_reader->StartCall();
  invocation_context->response_reader->Finish(&invocation_context->response, &invocation_context->status,
                                              invocation_context);
}

void RpcClientImpl::asyncHeartbeat(const HeartbeatRequest& request,
                                   InvocationContext<HeartbeatResponse>* invocation_context) {
  assert(invocation_context);
  invocation_context->response_reader =
      stub_->PrepareAsyncHeartbeat(&invocation_context->context, request, completion_queue_.get());
  invocation_context->response_reader->StartCall();
  invocation_context->response_reader->Finish(&invocation_context->response, &invocation_context->status,
                                              invocation_context);
}

void RpcClientImpl::asyncHealthCheck(const HealthCheckRequest& request,
                                     InvocationContext<HealthCheckResponse>* invocation_context) {
  assert(invocation_context);
  invocation_context->response_reader =
      stub_->PrepareAsyncHealthCheck(&invocation_context->context, request, completion_queue_.get());
  invocation_context->response_reader->StartCall();
  invocation_context->response_reader->Finish(&invocation_context->response, &invocation_context->status,
                                              invocation_context);
}

void RpcClientImpl::asyncEndTransaction(const EndTransactionRequest& request,
                                        InvocationContext<EndTransactionResponse>* invocation_context) {
  assert(invocation_context);
  invocation_context->response_reader =
      stub_->PrepareAsyncEndTransaction(&invocation_context->context, request, completion_queue_.get());
  invocation_context->response_reader->StartCall();
  invocation_context->response_reader->Finish(&invocation_context->response, &invocation_context->status,
                                              invocation_context);
}

bool RpcClientImpl::ok() const {
  return channel_ && grpc_connectivity_state::GRPC_CHANNEL_SHUTDOWN != channel_->GetState(false);
}

void RpcClientImpl::addMetadata(grpc::ClientContext& context,
                                const absl::flat_hash_map<std::string, std::string>& metadata) {
  for (const auto& entry : metadata) {
    context.AddMetadata(entry.first, entry.second);
  }
}

bool RpcClientImpl::needHeartbeat() {
  return need_heartbeat_;
}

void RpcClientImpl::needHeartbeat(bool need_heartbeat) {
  need_heartbeat_ = need_heartbeat;
}

void RpcClientImpl::asyncPollCommand(const PollCommandRequest& request,
                                     InvocationContext<PollCommandResponse>* invocation_context) {
  invocation_context->response_reader =
      stub_->PrepareAsyncPollCommand(&invocation_context->context, request, completion_queue_.get());
  invocation_context->response_reader->StartCall();
  invocation_context->response_reader->Finish(&invocation_context->response, &invocation_context->status,
                                              invocation_context);
}

grpc::Status RpcClientImpl::reportThreadStackTrace(grpc::ClientContext* context,
                                                   const ReportThreadStackTraceRequest& request,
                                                   ReportThreadStackTraceResponse* response) {
  return stub_->ReportThreadStackTrace(context, request, response);
}

grpc::Status RpcClientImpl::reportMessageConsumptionResult(grpc::ClientContext* context,
                                                           const ReportMessageConsumptionResultRequest& request,
                                                           ReportMessageConsumptionResultResponse* response) {
  return stub_->ReportMessageConsumptionResult(context, request, response);
}

grpc::Status RpcClientImpl::notifyClientTermination(grpc::ClientContext* context,
                                                    const NotifyClientTerminationRequest& request,
                                                    NotifyClientTerminationResponse* response) {
  return stub_->NotifyClientTermination(context, request, response);
}

void RpcClientImpl::asyncQueryOffset(const QueryOffsetRequest& request,
                                     InvocationContext<QueryOffsetResponse>* invocation_context) {
  assert(invocation_context);
  invocation_context->response_reader =
      stub_->PrepareAsyncQueryOffset(&invocation_context->context, request, completion_queue_.get());
  invocation_context->response_reader->StartCall();
  invocation_context->response_reader->Finish(&invocation_context->response, &invocation_context->status,
                                              invocation_context);
}

void RpcClientImpl::asyncPull(const PullMessageRequest& request,
                              InvocationContext<PullMessageResponse>* invocation_context) {
  invocation_context->response_reader =
      stub_->PrepareAsyncPullMessage(&invocation_context->context, request, completion_queue_.get());
  invocation_context->response_reader->StartCall();
  invocation_context->response_reader->Finish(&invocation_context->response, &invocation_context->status,
                                              invocation_context);
}

void RpcClientImpl::asyncForwardMessageToDeadLetterQueue(
    const ForwardMessageToDeadLetterQueueRequest& request,
    InvocationContext<ForwardMessageToDeadLetterQueueResponse>* invocation_context) {
  invocation_context->response_reader = stub_->PrepareAsyncForwardMessageToDeadLetterQueue(
      &invocation_context->context, request, completion_queue_.get());
  invocation_context->response_reader->StartCall();
  invocation_context->response_reader->Finish(&invocation_context->response, &invocation_context->status,
                                              invocation_context);
}

ROCKETMQ_NAMESPACE_END