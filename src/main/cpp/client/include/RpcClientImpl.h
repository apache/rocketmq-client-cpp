#pragma once

#include "RpcClient.h"

#include "absl/container/flat_hash_map.h"
#include <memory>

ROCKETMQ_NAMESPACE_BEGIN

class RpcClientImpl : public RpcClient, public std::enable_shared_from_this<RpcClientImpl> {
public:
  RpcClientImpl(std::shared_ptr<CompletionQueue> completion_queue, std::shared_ptr<Channel> channel,
                bool need_heartbeat = true)
      : completion_queue_(std::move(completion_queue)), channel_(std::move(channel)),
        stub_(rmq::MessagingService::NewStub(channel_)), need_heartbeat_(need_heartbeat) {}

  RpcClientImpl(const RpcClientImpl&) = delete;

  RpcClientImpl& operator=(const RpcClientImpl&) = delete;

  ~RpcClientImpl() override = default;

  void asyncQueryRoute(const QueryRouteRequest& request,
                       InvocationContext<QueryRouteResponse>* invocation_context) override;

  void asyncSend(const SendMessageRequest& request,
                 InvocationContext<SendMessageResponse>* invocation_context) override;

  void asyncQueryAssignment(const QueryAssignmentRequest& request,
                            InvocationContext<QueryAssignmentResponse>* invocation_context) override;

  std::shared_ptr<CompletionQueue>& completionQueue() override;

  void asyncReceive(const ReceiveMessageRequest& request,
                    InvocationContext<ReceiveMessageResponse>* invocation_context) override;

  void asyncAck(const AckMessageRequest& request, InvocationContext<AckMessageResponse>* invocation_context) override;

  void asyncNack(const NackMessageRequest& request,
                 InvocationContext<NackMessageResponse>* invocation_context) override;

  void asyncHeartbeat(const HeartbeatRequest& request,
                      InvocationContext<HeartbeatResponse>* invocation_context) override;

  void asyncHealthCheck(const HealthCheckRequest& request,
                        InvocationContext<HealthCheckResponse>* invocation_context) override;

  void asyncEndTransaction(const EndTransactionRequest& request,
                           InvocationContext<EndTransactionResponse>* invocation_context) override;

  void asyncMultiplexingCall(const MultiplexingRequest& request,
                             InvocationContext<MultiplexingResponse>* invocation_context) override;

  void asyncQueryOffset(const QueryOffsetRequest& request,
                        InvocationContext<QueryOffsetResponse>* invocation_context) override;

  void asyncPull(const PullMessageRequest& request,
                 InvocationContext<PullMessageResponse>* invocation_context) override;

  void asyncForwardMessageToDeadLetterQueue(
      const ForwardMessageToDeadLetterQueueRequest& request,
      InvocationContext<ForwardMessageToDeadLetterQueueResponse>* invocation_context) override;

  bool needHeartbeat() override;

  void needHeartbeat(bool need_heartbeat) override;

  bool ok() const override;

private:
  static void addMetadata(grpc::ClientContext& context, const absl::flat_hash_map<std::string, std::string>& metadata);

  std::shared_ptr<CompletionQueue> completion_queue_;
  std::shared_ptr<Channel> channel_;
  std::unique_ptr<rmq::MessagingService::Stub> stub_;
  std::chrono::milliseconds connect_timeout_{3000};
  bool need_heartbeat_{true};
};

using RpcClientSharedPtr = std::shared_ptr<RpcClient>;

ROCKETMQ_NAMESPACE_END