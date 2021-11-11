#include "RpcClientRemoting.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <system_error>

#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "apache/rocketmq/v1/definition.pb.h"
#include "apache/rocketmq/v1/service.pb.h"
#include "spdlog/spdlog.h"

#include "BrokerData.h"
#include "QueryRouteRequestHeader.h"
#include "QueueData.h"
#include "ResponseCode.h"

ROCKETMQ_NAMESPACE_BEGIN

void RpcClientRemoting::connect() {
  std::weak_ptr<RpcClientRemoting> self = shared_from_this();
  auto callback = std::bind(&RpcClientRemoting::onCallback, self, std::placeholders::_1);
  auto context = context_.lock();
  if (!context) {
    SPDLOG_WARN("Parent asio::io_context has destructed");
    return;
  }

  session_ = std::make_shared<RemotingSession>(context, endpoint_, callback);

  // Use blocking connect in development phase
  session_->connect(std::chrono::seconds(3), true);
}

bool RpcClientRemoting::ok() const {
  if (!session_) {
    return false;
  }

  return session_->state() == SessionState::Connected;
}

void RpcClientRemoting::asyncQueryRoute(const QueryRouteRequest& request,
                                        InvocationContext<QueryRouteResponse>* invocation_context) {
  assert(invocation_context);

  // Assign RequestCode
  invocation_context->request_code = RequestCode::QueryRoute;
  invocation_context->request = absl::make_unique<QueryRouteRequest>();
  invocation_context->request->CopyFrom(request);

  auto header = new QueryRouteRequestHeader();
  if (request.topic().resource_namespace().empty()) {
    header->topic(request.topic().name());
  } else {
    header->topic(absl::StrJoin({request.topic().resource_namespace(), request.topic().name()}, "%"));
  }

  auto command = RemotingCommand::createRequest(RequestCode::QueryRoute, header);

  {
    absl::MutexLock lk(&in_flight_requests_mtx_);
    in_flight_requests_.insert({command.opaque(), invocation_context});
  }

  std::error_code ec;
  session_->write(std::move(command), ec);

  if (ec) {
    grpc::Status aborted(grpc::StatusCode::ABORTED, ec.message());
    invocation_context->status = aborted;
    invocation_context->onCompletion(false);
  }
}

void RpcClientRemoting::onCallback(std::weak_ptr<RpcClientRemoting> rpc_client,
                                   const std::vector<RemotingCommand>& commands) {
  std::shared_ptr<RpcClientRemoting> remoting_client = rpc_client.lock();
  if (!remoting_client) {
    return;
  }

  for (const auto& command : commands) {
    remoting_client->processCommand(command);
  }
}

void RpcClientRemoting::processCommand(const RemotingCommand& command) {
  std::int32_t opaque = command.opaque();
  BaseInvocationContext* invocation_context = nullptr;
  {
    absl::MutexLock lk(&in_flight_requests_mtx_);
    if (in_flight_requests_.contains(opaque)) {
      invocation_context = in_flight_requests_[opaque];
      in_flight_requests_.erase(opaque);
    }
  }

  if (!invocation_context) {
    SPDLOG_WARN("Failed to look-up invocation-context through opaque[{}]", opaque);
    return;
  }

  switch (invocation_context->request_code) {
    case RequestCode::QueryRoute: {
      auto context = dynamic_cast<InvocationContext<QueryRouteResponse>*>(invocation_context);
      assert(nullptr != context);
      ResponseCode code = static_cast<ResponseCode>(command.code());
      switch (code) {
        case ResponseCode::Success: {
          google::protobuf::Struct root;
          auto data_ptr = reinterpret_cast<const char*>(command.body().data());
          google::protobuf::StringPiece json(data_ptr, command.body().size());

          /*
           * Sample JSON:
           * {
           *    "brokerDatas":[{"brokerName":"broker-a","brokerAddrs":{"0":"11.163.70.118:10911"},"cluster":"DefaultCluster","enableActingMaster":false}],
           *    "filterServerTable":{},
           *    "queueDatas":[{"brokerName":"broker-a","perm":6,"writeQueueNums":8,"readQueueNums":8,"topicSynFlag":0}]
           * }
           */
          auto status = google::protobuf::util::JsonStringToMessage(json, &root);
          if (!status.ok()) {
            SPDLOG_WARN("Failed to parse JSON: {}. Cause: {}", json.as_string(), status.message().as_string());
            return;
          }

          auto request = dynamic_cast<rmq::QueryRouteRequest*>(invocation_context->request.get());
          auto topic = new rmq::Resource();
          topic->CopyFrom(request->topic());

          // broker_name --> BrokerData
          absl::flat_hash_map<std::string, BrokerData> broker_data_map;
          // broker_name --> QueueData
          absl::flat_hash_map<std::string, QueueData> queue_data_map;

          const auto& fields = root.fields();
          if (fields.contains("brokerDatas")) {
            for (const auto& item : fields.at("brokerDatas").list_value().values()) {
              const auto& broker_data_struct = item.struct_value();
              auto&& broker_data = BrokerData::decode(broker_data_struct);
              broker_data_map.insert({broker_data.broker_name_, broker_data});
            }
          }

          if (fields.contains("queueDatas")) {
            for (const auto& item : fields.at("queueDatas").list_value().values()) {
              const auto& queue_data_struct = item.struct_value();
              auto&& queue_data = QueueData::decode(queue_data_struct);
              queue_data_map.insert({queue_data.broker_name_, queue_data});
            }
          }

          for (const auto& broker_entry : broker_data_map) {
            rmq::Broker broker;
            broker.set_name(broker_entry.second.broker_name_);

            const auto& addresses = broker_entry.second.broker_addresses_;
            if (addresses.empty()) {
              continue;
            }

            broker.mutable_endpoints()->set_scheme(rmq::AddressScheme::IPv4);

            std::int32_t broker_id = addresses.begin()->first;
            for (const auto& address_entry : addresses) {
              std::vector<std::string> segments = absl::StrSplit(address_entry.second, ":");
              if (2 == segments.size()) {
                auto address = new rmq::Address;
                address->set_host(segments[0]);
                address->set_port(std::stoi(segments[1]));
                broker.mutable_endpoints()->mutable_addresses()->AddAllocated(address);
              }

              if (address_entry.first < broker_id) {
                broker_id = address_entry.first;
              }
            }
            broker.set_id(broker_id);

            if (!queue_data_map.contains(broker_entry.first)) {
              continue;
            }

            const auto& queue_data = queue_data_map.at(broker_entry.first);

            // The following rule always holds: write_queue_num <= read_queue_num
            for (std::int32_t i = 0; i < queue_data.write_queue_number_; i++) {
              auto partition = new rmq::Partition();
              partition->set_permission(rmq::Permission::READ_WRITE);
              partition->mutable_broker()->CopyFrom(broker);
              partition->set_id(i);
              partition->mutable_topic()->CopyFrom(request->topic());
              context->response.mutable_partitions()->AddAllocated(partition);
            }

            for (std::int32_t i = queue_data.write_queue_number_; i < queue_data.read_queue_number_; i++) {
              auto partition = new rmq::Partition();
              partition->set_permission(rmq::Permission::READ);
              partition->mutable_broker()->CopyFrom(broker);
              partition->set_id(i);
              partition->mutable_topic()->CopyFrom(request->topic());
              context->response.mutable_partitions()->AddAllocated(partition);
            }
          }
          context->onCompletion(true);
          return;
        }
        case ResponseCode::InternalSystemError: {
          auto status = context->response.mutable_common()->mutable_status();
          status->set_code(static_cast<std::int32_t>(grpc::StatusCode::INTERNAL));
          status->set_message(command.remark());
          context->onCompletion(false);
          return;
        }
        case ResponseCode::TooManyRequests: {
          auto status = context->response.mutable_common()->mutable_status();
          status->set_code(static_cast<std::int32_t>(grpc::StatusCode::RESOURCE_EXHAUSTED));
          status->set_message(command.remark());
          context->onCompletion(false);
          return;
        }
      }
      break;
    }

    case RequestCode::SendMessage: {
      break;
    }

    case RequestCode::PopMessage: {
      break;
    }

    case RequestCode::AckMessage: {
      break;
    }

    case RequestCode::PullMessage: {
      break;
    }

    case RequestCode::Absent: {
      break;
    }
  }
}

void RpcClientRemoting::asyncSend(const SendMessageRequest& request,
                                  InvocationContext<SendMessageResponse>* invocation_context) {
}

void RpcClientRemoting::asyncQueryAssignment(const QueryAssignmentRequest& request,
                                             InvocationContext<QueryAssignmentResponse>* invocation_context) {
}

void RpcClientRemoting::asyncReceive(const ReceiveMessageRequest& request,
                                     InvocationContext<ReceiveMessageResponse>* invocation_context) {
}

void RpcClientRemoting::asyncAck(const AckMessageRequest& request,
                                 InvocationContext<AckMessageResponse>* invocation_context) {
}

void RpcClientRemoting::asyncNack(const NackMessageRequest& request,
                                  InvocationContext<NackMessageResponse>* invocation_context) {
}

void RpcClientRemoting::asyncHeartbeat(const HeartbeatRequest& request,
                                       InvocationContext<HeartbeatResponse>* invocation_context) {
}

void RpcClientRemoting::asyncHealthCheck(const HealthCheckRequest& request,
                                         InvocationContext<HealthCheckResponse>* invocation_context) {
}

void RpcClientRemoting::asyncEndTransaction(const EndTransactionRequest& request,
                                            InvocationContext<EndTransactionResponse>* invocation_context) {
}

void RpcClientRemoting::asyncPollCommand(const PollCommandRequest& request,
                                         InvocationContext<PollCommandResponse>* invocation_context) {
}

void RpcClientRemoting::asyncQueryOffset(const QueryOffsetRequest& request,
                                         InvocationContext<QueryOffsetResponse>* invocation_context) {
}

void RpcClientRemoting::asyncForwardMessageToDeadLetterQueue(
    const ForwardMessageToDeadLetterQueueRequest& request,
    InvocationContext<ForwardMessageToDeadLetterQueueResponse>* invocation_context) {
}

grpc::Status RpcClientRemoting::reportThreadStackTrace(grpc::ClientContext* context,
                                                       const ReportThreadStackTraceRequest& request,
                                                       ReportThreadStackTraceResponse* response) {
  return grpc::Status::OK;
}

grpc::Status RpcClientRemoting::reportMessageConsumptionResult(grpc::ClientContext* context,
                                                               const ReportMessageConsumptionResultRequest& request,
                                                               ReportMessageConsumptionResultResponse* response) {
  return grpc::Status::OK;
}

grpc::Status RpcClientRemoting::notifyClientTermination(grpc::ClientContext* context,
                                                        const NotifyClientTerminationRequest& request,
                                                        NotifyClientTerminationResponse* response) {
  return grpc::Status::OK;
}

ROCKETMQ_NAMESPACE_END