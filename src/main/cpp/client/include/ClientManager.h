#pragma once

#include "Client.h"
#include "ReceiveMessageCallback.h"
#include "Scheduler.h"
#include "TopAddressing.h"
#include "TopicRouteData.h"
#include "rocketmq/MQMessageExt.h"
#include <memory>

ROCKETMQ_NAMESPACE_BEGIN

using Metadata = absl::flat_hash_map<std::string, std::string>;

class ClientManager {
public:
  virtual ~ClientManager() = default;

  virtual void start() = 0;

  virtual void shutdown() = 0;

  virtual Scheduler& getScheduler() = 0;

  virtual TopAddressing& topAddressing() = 0;

  virtual std::shared_ptr<grpc::Channel> createChannel(const std::string& target_host) = 0;

  virtual void resolveRoute(const std::string& target_host, const Metadata& metadata, const QueryRouteRequest& request,
                            std::chrono::milliseconds timeout,
                            const std::function<void(bool, const TopicRouteDataPtr& ptr)>& cb) = 0;

  virtual void heartbeat(const std::string& target_host, const Metadata& metadata, const HeartbeatRequest& request,
                         std::chrono::milliseconds timeout,
                         const std::function<void(bool, const HeartbeatResponse&)>& cb) = 0;

  virtual void multiplexingCall(const std::string& target, const Metadata& metadata, const MultiplexingRequest& request,
                                std::chrono::milliseconds timeout,
                                const std::function<void(const InvocationContext<MultiplexingResponse>*)>& cb) = 0;

  virtual bool wrapMessage(const rmq::Message& item, MQMessageExt& message_ext) = 0;

  virtual void ack(const std::string& target_host, const Metadata& metadata, const AckMessageRequest& request,
                   std::chrono::milliseconds timeout, const std::function<void(bool)>& cb) = 0;

  virtual void nack(const std::string& target_host, const Metadata& metadata, const NackMessageRequest& request,
                    std::chrono::milliseconds timeout, const std::function<void(bool)>& callback) = 0;

  virtual void forwardMessageToDeadLetterQueue(
      const std::string& target_host, const Metadata& metadata, const ForwardMessageToDeadLetterQueueRequest& request,
      std::chrono::milliseconds timeout,
      const std::function<void(const InvocationContext<ForwardMessageToDeadLetterQueueResponse>*)>& cb) = 0;

  virtual void endTransaction(const std::string& target_host, const Metadata& metadata,
                              const EndTransactionRequest& request, std::chrono::milliseconds timeout,
                              const std::function<void(bool, const EndTransactionResponse&)>& cb) = 0;

  virtual void queryOffset(const std::string& target_host, const Metadata& metadata, const QueryOffsetRequest& request,
                           std::chrono::milliseconds timeout,
                           const std::function<void(bool, const QueryOffsetResponse&)>& cb) = 0;

  virtual void
  healthCheck(const std::string& target_host, const Metadata& metadata, const HealthCheckRequest& request,
              std::chrono::milliseconds timeout,
              const std::function<void(const std::string&, const InvocationContext<HealthCheckResponse>*)>& cb) = 0;

  virtual void addClientObserver(std::weak_ptr<Client> client) = 0;

  virtual void queryAssignment(const std::string& target, const Metadata& metadata,
                               const QueryAssignmentRequest& request, std::chrono::milliseconds timeout,
                               const std::function<void(bool, const QueryAssignmentResponse&)>& cb) = 0;

  virtual void receiveMessage(const std::string& target, const Metadata& metadata, const ReceiveMessageRequest& request,
                              std::chrono::milliseconds timeout, const std::shared_ptr<ReceiveMessageCallback>& cb) = 0;

  virtual bool send(const std::string& target_host, const Metadata& metadata, SendMessageRequest& request,
                    SendCallback* cb) = 0;

  virtual void processPullResult(const grpc::ClientContext& client_context, const PullMessageResponse& response,
                                 ReceiveMessageResult& result, const std::string& target_host) = 0;

  virtual void pullMessage(const std::string& target_host, const Metadata& metadata, const PullMessageRequest& request,
                           std::chrono::milliseconds timeout,
                           const std::function<void(const InvocationContext<PullMessageResponse>*)>& cb) = 0;
};

using ClientManagerPtr = std::shared_ptr<ClientManager>;

ROCKETMQ_NAMESPACE_END