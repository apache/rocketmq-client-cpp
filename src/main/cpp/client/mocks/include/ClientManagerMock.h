#pragma once

#include "ClientManager.h"
#include "gmock/gmock.h"
#include <chrono>

ROCKETMQ_NAMESPACE_BEGIN

class ClientManagerMock : public ClientManager {
public:
  MOCK_METHOD(void, start, (), (override));

  MOCK_METHOD(void, shutdown, (), (override));

  MOCK_METHOD(Scheduler&, getScheduler, (), (override));

  MOCK_METHOD(TopAddressing&, topAddressing, (), (override));

  MOCK_METHOD((std::shared_ptr<grpc::Channel>), createChannel, (const std::string&), (override));

  MOCK_METHOD(void, resolveRoute,
              (const std::string&, const Metadata&, const QueryRouteRequest&, std::chrono::milliseconds,
               (const std::function<void(bool, const TopicRouteDataPtr&)>&)),
              (override));

  MOCK_METHOD(void, heartbeat,
              (const std::string&, const Metadata&, const HeartbeatRequest&, std::chrono::milliseconds,
               (const std::function<void(bool, const HeartbeatResponse&)>&)),
              (override));

  MOCK_METHOD(void, multiplexingCall,
              (const std::string&, const Metadata&, const MultiplexingRequest&, std::chrono::milliseconds,
               (const std::function<void(const InvocationContext<MultiplexingResponse>*)>&)),
              (override));

  MOCK_METHOD(bool, wrapMessage, (const rmq::Message&, MQMessageExt&), (override));

  MOCK_METHOD(void, ack,
              (const std::string&, const Metadata&, const AckMessageRequest&, std::chrono::milliseconds,
               (const std::function<void(bool)>&)),
              (override));

  MOCK_METHOD(void, nack,
              (const std::string&, const Metadata&, const NackMessageRequest&, std::chrono::milliseconds,
               (const std::function<void(bool)>&)),
              (override));

  MOCK_METHOD(void, forwardMessageToDeadLetterQueue,
              (const std::string&, const Metadata&, const ForwardMessageToDeadLetterQueueRequest&,
               std::chrono::milliseconds,
               (const std::function<void(const InvocationContext<ForwardMessageToDeadLetterQueueResponse>*)>&)),
              (override));

  MOCK_METHOD(void, endTransaction,
              (const std::string&, const Metadata&, const EndTransactionRequest&, std::chrono::milliseconds,
               (const std::function<void(bool, const EndTransactionResponse&)>&)),
              (override));

  MOCK_METHOD(void, queryOffset,
              (const std::string&, const Metadata&, const QueryOffsetRequest&, std::chrono::milliseconds,
               (const std::function<void(bool, const QueryOffsetResponse&)>&)),
              (override));

  MOCK_METHOD(void, healthCheck,
              (const std::string&, const Metadata&, const HealthCheckRequest&, std::chrono::milliseconds,
               (const std::function<void(const std::string&, const InvocationContext<HealthCheckResponse>*)>&)),
              (override));

  MOCK_METHOD(void, addClientObserver, (std::weak_ptr<Client>), (override));

  MOCK_METHOD(void, queryAssignment,
              (const std::string& target, const Metadata&, const QueryAssignmentRequest&, std::chrono::milliseconds,
               (const std::function<void(bool, const QueryAssignmentResponse&)>&)),
              (override));

  MOCK_METHOD(void, receiveMessage,
              (const std::string&, const Metadata&, const ReceiveMessageRequest&, std::chrono::milliseconds,
               (const std::shared_ptr<ReceiveMessageCallback>&)),
              (override));

  MOCK_METHOD(bool, send, (const std::string&, const Metadata&, SendMessageRequest&, SendCallback*), (override));

  MOCK_METHOD(void, processPullResult,
              (const grpc::ClientContext&, const PullMessageResponse&, ReceiveMessageResult&, const std::string&),
              (override));

  MOCK_METHOD(void, pullMessage,
              (const std::string&, const Metadata&, const PullMessageRequest&, std::chrono::milliseconds,
               (const std::function<void(const InvocationContext<PullMessageResponse>*)>&)),
              (override));

  MOCK_METHOD(bool, notifyClientTermination,
              (const std::string&, const Metadata&, const NotifyClientTerminationRequest&, std::chrono::milliseconds),
              (override));
};

ROCKETMQ_NAMESPACE_END