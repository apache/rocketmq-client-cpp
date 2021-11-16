#pragma once

#include <apache/rocketmq/v1/definition.pb.h>
#include <cstdint>
#include <memory>

#include "InvocationContext.h"
#include "RemotingCommand.h"
#include "absl/base/thread_annotations.h"
#include "absl/strings/str_split.h"
#include "absl/synchronization/mutex.h"
#include "asio.hpp"

#include "RemotingSession.h"
#include "RpcClient.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class RpcClientRemoting : public RpcClient, public std::enable_shared_from_this<RpcClientRemoting> {
public:
  RpcClientRemoting(std::weak_ptr<asio::io_context> context, const std::string& endpoint)
      : context_(std::move(context)) {
    if (absl::StartsWith(endpoint, "ipv4:")) {
      auto view = absl::StripPrefix(endpoint, "ipv4:");
      endpoint_ = std::string(view.data(), view.length());
    } else {
      endpoint_ = endpoint;
    }
  }

  void connect() override;

  void asyncQueryRoute(const QueryRouteRequest& request,
                       InvocationContext<QueryRouteResponse>* invocation_context) override;

  void asyncSend(const SendMessageRequest& request,
                 InvocationContext<SendMessageResponse>* invocation_context) override;

  void asyncQueryAssignment(const QueryAssignmentRequest& request,
                            InvocationContext<QueryAssignmentResponse>* invocation_context) override;

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

  void asyncPollCommand(const PollCommandRequest& request,
                        InvocationContext<PollCommandResponse>* invocation_context) override;

  void asyncQueryOffset(const QueryOffsetRequest& request,
                        InvocationContext<QueryOffsetResponse>* invocation_context) override;

  void asyncPull(const PullMessageRequest& request,
                 InvocationContext<PullMessageResponse>* invocation_context) override;

  void asyncForwardMessageToDeadLetterQueue(
      const ForwardMessageToDeadLetterQueueRequest& request,
      InvocationContext<ForwardMessageToDeadLetterQueueResponse>* invocation_context) override;

  grpc::Status reportThreadStackTrace(grpc::ClientContext* context, const ReportThreadStackTraceRequest& request,
                                      ReportThreadStackTraceResponse* response) override;

  grpc::Status reportMessageConsumptionResult(grpc::ClientContext* context,
                                              const ReportMessageConsumptionResultRequest& request,
                                              ReportMessageConsumptionResultResponse* response) override;

  grpc::Status notifyClientTermination(grpc::ClientContext* context, const NotifyClientTerminationRequest& request,
                                       NotifyClientTerminationResponse* response) override;

  /**
   * Indicate if heartbeat is required.
   * @return true if periodic heartbeat is required; false otherwise.
   */
  bool needHeartbeat() override {
    return need_heartbeat_;
  }

  void needHeartbeat(bool need_heartbeat) override {
    need_heartbeat_ = need_heartbeat;
  }

  /**
   * Indicate if current client connection state is OK or recoverable.
   *
   * @return true if underlying connection is OK or recoverable; false otherwise.
   */
  bool ok() const override;

private:
  std::weak_ptr<asio::io_context> context_;
  std::string endpoint_;
  std::shared_ptr<RemotingSession> session_;
  bool need_heartbeat_{false};

  absl::flat_hash_map<std::int32_t, BaseInvocationContext*> in_flight_requests_ GUARDED_BY(in_flight_requests_mtx_);
  absl::Mutex in_flight_requests_mtx_;

  void write(RemotingCommand command, BaseInvocationContext* invocation_context)
      LOCKS_EXCLUDED(in_flight_requests_mtx_);

  static void onCallback(std::weak_ptr<RpcClientRemoting> rpc_client, const std::vector<RemotingCommand>& commands);

  void processCommand(const RemotingCommand& command) LOCKS_EXCLUDED(in_flight_requests_mtx_);

  void handleQueryRoute(const RemotingCommand& command, BaseInvocationContext* context);

  void handleSendMessage(const RemotingCommand& command, BaseInvocationContext* context);

  void handlePopMessage(const RemotingCommand& command, BaseInvocationContext* context);

  void handleAckMessage(const RemotingCommand& command, BaseInvocationContext* context);

  void handleHeartbeat(const RemotingCommand& command, BaseInvocationContext* context);

  void handlePullMessage(const RemotingCommand& command, BaseInvocationContext* context);

  void decodeMessages(google::protobuf::RepeatedPtrField<rmq::Message>* messages, const std::uint8_t* base,
                      std::size_t limit);
};

ROCKETMQ_NAMESPACE_END