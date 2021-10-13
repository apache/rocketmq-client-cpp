#pragma once

#include "ClientManager.h"
#include "gmock/gmock.h"
#include <chrono>
#include <system_error>

ROCKETMQ_NAMESPACE_BEGIN

class ClientManagerMock : public ClientManager {
public:
  MOCK_METHOD(void, start, (), (override));

  MOCK_METHOD(void, shutdown, (), (override));

  MOCK_METHOD(Scheduler&, getScheduler, (), (override));

  MOCK_METHOD((std::shared_ptr<grpc::Channel>), createChannel, (const std::string&), (override));

  MOCK_METHOD(void, resolveRoute,
              (const std::string&, const Metadata&, const QueryRouteRequest&, std::chrono::milliseconds,
               (const std::function<void(const std::error_code&, const TopicRouteDataPtr&)>&)),
              (override));

  MOCK_METHOD(void, heartbeat,
              (const std::string&, const Metadata&, const HeartbeatRequest&, std::chrono::milliseconds,
               (const std::function<void(const std::error_code&, const HeartbeatResponse&)>&)),
              (override));

  MOCK_METHOD(void, pollCommand,
              (const std::string&, const Metadata&, const PollCommandRequest&, std::chrono::milliseconds,
               const std::function<void(const InvocationContext<PollCommandResponse>*)>&),
              (override));

  MOCK_METHOD(bool, wrapMessage, (const rmq::Message&, MQMessageExt&), (override));

  MOCK_METHOD(void, ack,
              (const std::string&, const Metadata&, const AckMessageRequest&, std::chrono::milliseconds,
               (const std::function<void(const std::error_code&)>&)),
              (override));

  MOCK_METHOD(void, nack,
              (const std::string&, const Metadata&, const NackMessageRequest&, std::chrono::milliseconds,
               (const std::function<void(const std::error_code&)>&)),
              (override));

  MOCK_METHOD(void, forwardMessageToDeadLetterQueue,
              (const std::string&, const Metadata&, const ForwardMessageToDeadLetterQueueRequest&,
               std::chrono::milliseconds,
               (const std::function<void(const InvocationContext<ForwardMessageToDeadLetterQueueResponse>*)>&)),
              (override));

  MOCK_METHOD(void, endTransaction,
              (const std::string&, const Metadata&, const EndTransactionRequest&, std::chrono::milliseconds,
               (const std::function<void(const std::error_code&, const EndTransactionResponse&)>&)),
              (override));

  MOCK_METHOD(void, queryOffset,
              (const std::string&, const Metadata&, const QueryOffsetRequest&, std::chrono::milliseconds,
               (const std::function<void(const std::error_code&, const QueryOffsetResponse&)>&)),
              (override));

  MOCK_METHOD(void, healthCheck,
              (const std::string&, const Metadata&, const HealthCheckRequest&, std::chrono::milliseconds,
               (const std::function<void(const std::error_code&, const InvocationContext<HealthCheckResponse>*)>&)),
              (override));

  MOCK_METHOD(void, addClientObserver, (std::weak_ptr<Client>), (override));

  MOCK_METHOD(void, queryAssignment,
              (const std::string& target, const Metadata&, const QueryAssignmentRequest&, std::chrono::milliseconds,
               (const std::function<void(const std::error_code&, const QueryAssignmentResponse&)>&)),
              (override));

  MOCK_METHOD(void, receiveMessage,
              (const std::string&, const Metadata&, const ReceiveMessageRequest&, std::chrono::milliseconds,
               (const std::shared_ptr<ReceiveMessageCallback>&)),
              (override));

  MOCK_METHOD(bool, send, (const std::string&, const Metadata&, SendMessageRequest&, SendCallback*), (override));

  MOCK_METHOD(void, pullMessage,
              (const std::string&, const Metadata&, const PullMessageRequest&, std::chrono::milliseconds,
               (const std::function<void(const std::error_code&, const ReceiveMessageResult&)>&)),
              (override));

  MOCK_METHOD(std::error_code, notifyClientTermination,
              (const std::string&, const Metadata&, const NotifyClientTerminationRequest&, std::chrono::milliseconds),
              (override));

  MOCK_METHOD(std::error_code, reportThreadStackTrace,
              (const std::string&, const Metadata&, const ReportThreadStackTraceRequest&, std::chrono::milliseconds),
              (override));

  MOCK_METHOD(std::error_code, reportMessageConsumptionResult,
              (const std::string&, const Metadata&, const ReportMessageConsumptionResultRequest&,
               std::chrono::milliseconds),
              (override));

  MOCK_METHOD(State, state, (), (const override));

  MOCK_METHOD(void, submit, (std::function<void()>), (override));
};

ROCKETMQ_NAMESPACE_END