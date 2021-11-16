#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "apache/rocketmq/v1/definition.pb.h"
#include "apache/rocketmq/v1/service.pb.h"
#include "google/rpc/code.pb.h"
#include "google/rpc/status.pb.h"

#include "AckMessageRequestHeader.h"
#include "BrokerData.h"
#include "ConsumeFromWhere.h"
#include "HeartbeatData.h"
#include "InvocationContext.h"
#include "LoggerImpl.h"
#include "MessageVersion.h"
#include "MixAll.h"
#include "PopMessageRequestHeader.h"
#include "PopMessageResponseHeader.h"
#include "PullMessageRequestHeader.h"
#include "PullMessageResponseHeader.h"
#include "QueryRouteRequestHeader.h"
#include "QueueData.h"
#include "RemotingCommand.h"
#include "RemotingConstants.h"
#include "RemotingHelper.h"
#include "ResponseCode.h"
#include "RpcClient.h"
#include "RpcClientRemoting.h"
#include "SendMessageRequestHeader.h"
#include "SendMessageResponseHeader.h"
#include "rocketmq/MessageModel.h"
#include "rocketmq/MessageType.h"

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

void RpcClientRemoting::write(RemotingCommand command, BaseInvocationContext* invocation_context) {
  {
    absl::MutexLock lk(&in_flight_requests_mtx_);
    in_flight_requests_.insert({command.opaque(), invocation_context});
  }

  SPDLOG_DEBUG("Writing RemotingCommand to {}", invocation_context->remote_address);
  std::error_code ec;
  session_->write(std::move(command), ec);

  if (ec) {
    SPDLOG_WARN("Failed to write request to {}", invocation_context->remote_address);
    grpc::Status aborted(grpc::StatusCode::ABORTED, ec.message());
    invocation_context->status = aborted;
    invocation_context->onCompletion(false);
  }
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
  write(std::move(command), invocation_context);
}

void RpcClientRemoting::onCallback(std::weak_ptr<RpcClientRemoting> rpc_client,
                                   const std::vector<RemotingCommand>& commands) {
  std::shared_ptr<RpcClientRemoting> remoting_client = rpc_client.lock();
  if (!remoting_client) {
    return;
  }

  SPDLOG_DEBUG("Received {} remoting commands from {}", commands.size(), remoting_client->endpoint_);
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
      SPDLOG_DEBUG("Erased invocation-context[opaque={}]", opaque);
    }
  }

  if (!invocation_context) {
    SPDLOG_WARN("Failed to look-up invocation-context through opaque[{}]", opaque);
    return;
  }

  switch (invocation_context->request_code) {
    case RequestCode::QueryRoute: {
      handleQueryRoute(command, invocation_context);
      break;
    }

    case RequestCode::SendMessage: {
      handleSendMessage(command, invocation_context);
      break;
    }

    case RequestCode::PopMessage: {
      handlePopMessage(command, invocation_context);
      break;
    }

    case RequestCode::AckMessage: {
      handleAckMessage(command, invocation_context);
      break;
    }

    case RequestCode::PullMessage: {
      handlePullMessage(command, invocation_context);
      break;
    }

    case RequestCode::Heartbeat: {
      handleHeartbeat(command, invocation_context);
      break;
    }

    case RequestCode::Absent: {
      break;
    }
  }
}

void RpcClientRemoting::handleQueryRoute(const RemotingCommand& command, BaseInvocationContext* invocation_context) {
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
          std::vector<std::string> segments = absl::StrSplit(address_entry.second, ':');
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
    case ResponseCode::TopicNotFound: {
      auto status = context->response.mutable_common()->mutable_status();
      status->set_code(static_cast<std::int32_t>(grpc::StatusCode::NOT_FOUND));
      status->set_message(command.remark());
      context->onCompletion(true);
      return;
    }
    default: {
      auto status = context->response.mutable_common()->mutable_status();
      status->set_code(static_cast<std::int32_t>(grpc::StatusCode::UNIMPLEMENTED));
      status->set_message(command.remark());
      context->onCompletion(true);
      return;
    }
  }
}

void RpcClientRemoting::handleSendMessage(const RemotingCommand& command, BaseInvocationContext* invocation_context) {
  SPDLOG_DEBUG("Process send message response command. Code: {}, Remark: {}", command.code(), command.remark());
  auto context = dynamic_cast<InvocationContext<SendMessageResponse>*>(invocation_context);
  auto response_code = static_cast<ResponseCode>(command.code());
  auto status = context->response.mutable_common()->mutable_status();
  status->set_message(command.remark());
  switch (response_code) {
    case ResponseCode::Success: {
      auto header = dynamic_cast<const SendMessageResponseHeader*>(command.extHeader());
      context->response.set_message_id(header->messageId());
      context->response.set_transaction_id(header->transactionId());
      context->onCompletion(true);
      return;
    }

    case ResponseCode::InternalSystemError: {
      SPDLOG_ERROR("Internal error. Remark: {}", command.remark());
      status->set_code(static_cast<std::int32_t>(grpc::StatusCode::INTERNAL));
      break;
    }

    case ResponseCode::TooManyRequests: {
      SPDLOG_ERROR("Too many requests. Remark: {}", command.remark());
      status->set_code(static_cast<std::int32_t>(grpc::StatusCode::RESOURCE_EXHAUSTED));
      break;
    }
    case ResponseCode::MessageIllegal: {
      SPDLOG_ERROR("Message being sent is illegal. Remark: {}", command.remark());
      status->set_code(static_cast<std::int32_t>(grpc::StatusCode::INVALID_ARGUMENT));
      break;
    }
    default: {
      // TODO: error-handling
      SPDLOG_WARN("Unsupported code: {}. Remark: {}", command.code(), command.remark());
      status->set_code(static_cast<std::int32_t>(grpc::StatusCode::UNKNOWN));
      break;
    }
  }
  context->onCompletion(true);
}

void RpcClientRemoting::handlePopMessage(const RemotingCommand& command, BaseInvocationContext* invocation_context) {
  SPDLOG_DEBUG("Handle pop-message response. Code: {}, Remark: {}", command.code(), command.remark());
  auto context = dynamic_cast<InvocationContext<ReceiveMessageResponse>*>(invocation_context);
  auto response_code = static_cast<ResponseCode>(command.code());
  auto status = context->response.mutable_common()->mutable_status();
  status->set_message(command.remark());

  switch (response_code) {
    case ResponseCode::Success: {
      const auto header = dynamic_cast<const PopMessageResponseHeader*>(command.extHeader());
      auto invisible_duration = context->response.mutable_invisible_duration();
      invisible_duration->set_seconds(header->invisibleTimeInMillis() / 1000);
      invisible_duration->set_nanos((header->invisibleTimeInMillis() % 1000) * 1e6);

      auto pop_time = context->response.mutable_delivery_timestamp();
      pop_time->set_seconds(header->popTime() / 1000);
      pop_time->set_nanos(header->popTime() % 1000 * 1e6);

      auto messages = context->response.mutable_messages();

      const std::uint8_t* base = command.body().data();
      std::size_t body_length = command.body().size();
      decodeMessages(messages, base, body_length);
      SPDLOG_DEBUG("Received {} messages from server {} by pop", messages->size(), context->remote_address);
      break;
    }
    case ResponseCode::InternalSystemError: {
      status->set_code(static_cast<std::int32_t>(grpc::StatusCode::INTERNAL));
      break;
    }
    case ResponseCode::TooManyReceiveRequests: {
      status->set_code(static_cast<std::int32_t>(grpc::StatusCode::RESOURCE_EXHAUSTED));
      break;
    }
    case ResponseCode::ReceiveMessageTimeout: {
      status->set_code(static_cast<std::int32_t>(grpc::StatusCode::DEADLINE_EXCEEDED));
      break;
    }
    default: {
      SPDLOG_WARN("Unsupported code: {}, Remark: {}", command.code(), command.remark());
      status->set_code(static_cast<std::int32_t>(grpc::StatusCode::UNIMPLEMENTED));
      break;
    }
  }
  context->onCompletion(true);
}

void RpcClientRemoting::handleAckMessage(const RemotingCommand& command, BaseInvocationContext* invocation_context) {
  SPDLOG_DEBUG("Handle ack message response. Code: {}, remark: {}", command.code(), command.remark());
  auto context = dynamic_cast<InvocationContext<AckMessageResponse>*>(invocation_context);
  auto response_code = static_cast<ResponseCode>(command.code());
  auto status = context->response.mutable_common()->mutable_status();
  status->set_message(command.remark());

  switch (response_code) {
    case ResponseCode::Success: {
      SPDLOG_DEBUG("Acked message. Associated Ack Request: {}", context->request->ShortDebugString());
      break;
    }

    case ResponseCode::InternalSystemError: {
      SPDLOG_WARN("Failed to ack message, cause: {}", command.remark());
      status->set_code(static_cast<std::int32_t>(google::rpc::Code::INTERNAL));
      break;
    }

    case ResponseCode::TooManyRequests: {
      SPDLOG_WARN("Failed to ack message, cause: {}", command.remark());
      status->set_code(static_cast<std::int32_t>(google::rpc::Code::RESOURCE_EXHAUSTED));
      break;
    }
    default: {
      SPDLOG_WARN("Failed to ack message, cause: {}", command.remark());
      status->set_code(static_cast<std::int32_t>(google::rpc::Code::UNIMPLEMENTED));
      break;
    }
  }
  invocation_context->onCompletion(true);
}

void RpcClientRemoting::decodeMessages(google::protobuf::RepeatedPtrField<rmq::Message>* messages,
                                       const std::uint8_t* base, std::size_t body_length) {
  std::uint32_t offset = 0;
  while (offset < body_length - 1) {
    SPDLOG_DEBUG("Decode message out of pop-response. offset: {}, command.body.size: {}", offset, body_length);
    auto message = new rmq::Message();
    std::error_code ec;
    // Store size
    std::int32_t store_size = RemotingHelper::readBigEndian<std::int32_t>(base + offset, ec);
    offset += sizeof(std::int32_t);
    if (ec) {
      SPDLOG_WARN("Failed to decode store_size out of pop message response. Cause: {}", ec.message());
    }
    (void)store_size;

    // Magic code
    std::int32_t magic_code = RemotingHelper::readBigEndian<std::int32_t>(base + offset, ec);
    offset += sizeof(std::int32_t);
    if (ec) {
      SPDLOG_WARN("Failed to decode magic code out of pop response. Cause: {}", ec.message());
    }
    SPDLOG_DEBUG("MagicCode: {}", magic_code);

    MessageVersion message_version = messageVersionOf(magic_code);
    if (MessageVersion::Unset == message_version) {
      SPDLOG_WARN("Yuck, got an illegal magic code: {}", magic_code);
      break;
    }

    // Body CRC
    std::int32_t body_crc = RemotingHelper::readBigEndian<std::int32_t>(base + offset, ec);
    offset += sizeof(std::int32_t);

    // Queue ID
    std::int32_t queue_id = RemotingHelper::readBigEndian<std::int32_t>(base + offset, ec);
    offset += sizeof(std::int32_t);
    message->mutable_system_attribute()->set_partition_id(queue_id);

    std::int32_t flag = RemotingHelper::readBigEndian<std::int32_t>(base + offset, ec);
    offset += sizeof(std::int32_t);
    (void)flag;

    std::int64_t queue_offset = RemotingHelper::readBigEndian<std::int64_t>(base + offset, ec);
    offset += sizeof(std::int64_t);
    message->mutable_system_attribute()->set_partition_offset(queue_offset);

    std::int64_t commit_log_offset = RemotingHelper::readBigEndian<std::int64_t>(base + offset, ec);
    offset += sizeof(std::int64_t);
    (void)commit_log_offset;

    std::int32_t system_flag = RemotingHelper::readBigEndian<std::int32_t>(base + offset, ec);
    offset += sizeof(std::int32_t);
    {
      if ((RemotingConstants::FlagCompression & system_flag) == RemotingConstants::FlagCompression) {
        message->mutable_system_attribute()->set_body_encoding(rmq::Encoding::GZIP);
      }
    }

    std::int64_t born_timestamp = RemotingHelper::readBigEndian<std::int64_t>(base + offset, ec);
    offset += sizeof(std::int64_t);
    message->mutable_system_attribute()->mutable_born_timestamp()->set_seconds(born_timestamp / 1000);
    message->mutable_system_attribute()->mutable_born_timestamp()->set_nanos((born_timestamp % 1000) * 1e6);

    std::int32_t born_host_ip = RemotingHelper::readBigEndian<std::int32_t>(base + offset, ec);
    offset += sizeof(std::int32_t);
    (void)born_host_ip;

    std::int32_t born_host_port = RemotingHelper::readBigEndian<std::int32_t>(base + offset, ec);
    offset += sizeof(std::int32_t);
    (void)born_host_port;

    std::int64_t store_timestamp = RemotingHelper::readBigEndian<std::int64_t>(base + offset, ec);
    offset += sizeof(std::int64_t);
    message->mutable_system_attribute()->mutable_store_timestamp()->set_seconds(store_timestamp / 1000);
    message->mutable_system_attribute()->mutable_store_timestamp()->set_nanos((store_timestamp % 1000) * 1e6);

    std::int32_t store_host_ip = RemotingHelper::readBigEndian<std::int32_t>(base + offset, ec);
    offset += sizeof(std::int32_t);
    (void)store_host_ip;

    std::int32_t store_host_port = RemotingHelper::readBigEndian<std::int32_t>(base + offset, ec);
    offset += sizeof(std::int32_t);
    (void)store_host_port;

    std::int32_t reconsume_times = RemotingHelper::readBigEndian<std::int32_t>(base + offset, ec);
    offset += sizeof(std::int32_t);
    (void)reconsume_times;

    std::int64_t prepare_transaction_offset = RemotingHelper::readBigEndian<std::int64_t>(base + offset, ec);
    offset += sizeof(std::int64_t);
    (void)prepare_transaction_offset;

    std::int32_t body_length = RemotingHelper::readBigEndian<std::int32_t>(base + offset, ec);
    offset += sizeof(std::int32_t);

    if (body_length > 0) {
      std::string body;
      body.resize(body_length);
      memcpy(const_cast<char*>(body.data()), base + offset, body_length);
      offset += body_length;
      message->set_body(body);

      std::string calculated_crc;
      MixAll::crc32(body, calculated_crc);
      message->mutable_system_attribute()->mutable_body_digest()->set_type(rmq::DigestType::CRC32);
      message->mutable_system_attribute()->mutable_body_digest()->set_checksum(calculated_crc);

      if (body_crc) {
        std::string crc = absl::StrFormat("%X", body_crc);
        if (crc != calculated_crc) {
          SPDLOG_WARN("Calculated CRC: {}, CRC from broker in big endian: {}, its hex: {}, body: {}, len(body): {}",
                      calculated_crc, body_crc, crc, body, body.length());
        }
      }
    }

    std::size_t topic_length = 0;
    switch (message_version) {
      case MessageVersion::V1: {
        // The following byte represents length of the topic
        topic_length = *(base + offset);
        offset += 1;
        break;
      }
      case MessageVersion::V2: {
        // The following two bytes represents length of the topic
        topic_length = RemotingHelper::readBigEndian<std::int16_t>(base + offset, ec);
        offset += sizeof(std::int16_t);
        break;
      }
      default: {
        SPDLOG_WARN("Fatal error while decode body of pop response into messages. Caused by unsupported magic code: {}",
                    magic_code);
        break;
      }
    }

    std::string topic;
    topic.resize(topic_length);
    memcpy(const_cast<char*>(topic.data()), base + offset, topic_length);
    offset += topic_length;
    SPDLOG_DEBUG("Topic: {}", topic);
    if (absl::StrContains(topic, "%")) {
      std::vector<std::string> segments = absl::StrSplit(topic, '%');
      if (segments.size() == 2) {
        message->mutable_topic()->set_resource_namespace(segments[0]);
        message->mutable_topic()->set_name(segments[1]);
      } else {
        SPDLOG_WARN("[BUG] more cases to handle in terms of topic");
      }
    } else {
      message->mutable_topic()->set_name(topic);
    }

    std::int16_t properties_length = RemotingHelper::readBigEndian<std::int16_t>(base + offset, ec);
    offset += sizeof(std::int16_t);

    std::string properties;
    properties.resize(properties_length);
    memcpy(const_cast<char*>(properties.data()), base + offset, properties_length);
    offset += properties_length;

    absl::flat_hash_map<std::string, std::string> properties_map =
        RemotingHelper::stringToMessageProperties(properties);
    SPDLOG_DEBUG("Message properties: {}", absl::StrJoin(properties_map, ",", absl::PairFormatter("=")));
    for (const auto& entry : properties_map) {
      if (RemotingConstants::Keys == entry.first) {
        std::vector<std::string> keys = absl::StrSplit(entry.second, RemotingConstants::KeySeparator);
        message->mutable_system_attribute()->mutable_keys()->Add(keys.begin(), keys.end());
        continue;
      }

      if (RemotingConstants::Tags == entry.first) {
        if (!entry.second.empty()) {
          message->mutable_system_attribute()->set_tag(entry.second);
        }
        continue;
      }

      if (RemotingConstants::PopCk == entry.first) {
        message->mutable_system_attribute()->set_receipt_handle(entry.second);
        SPDLOG_DEBUG("Receipt-Handle: {}", entry.second);
        continue;
      }

      // TODO: check all other system properties.
      message->mutable_user_attribute()->insert({entry.first, entry.second});
    }
    messages->AddAllocated(message);
  }
}

void RpcClientRemoting::handlePullMessage(const RemotingCommand& command, BaseInvocationContext* invocation_context) {
  SPDLOG_DEBUG("Handle pull message response. Code: {}, remark: {}", command.code(), command.remark());
  auto context = dynamic_cast<InvocationContext<PullMessageResponse>*>(invocation_context);
  auto response_code = static_cast<ResponseCode>(command.code());
  auto status = context->response.mutable_common()->mutable_status();
  status->set_message(command.remark());

  auto response_header = dynamic_cast<const PullMessageResponseHeader*>(command.extHeader());
  if (response_header) {
    context->response.set_next_offset(response_header->next_begin_offset_);
    context->response.set_min_offset(response_header->min_offset_);
    context->response.set_max_offset(response_header->max_offset_);
  }

  switch (response_code) {
    case ResponseCode::Success: {
      auto messages = context->response.mutable_messages();
      const std::uint8_t* base = command.body().data();
      std::size_t body_length = command.body().size();
      decodeMessages(messages, base, body_length);
      SPDLOG_DEBUG("Received {} messages from server: {} by pull", messages->size(), context->remote_address);
      break;
    }
    case ResponseCode::PullNotFound: {
      status->set_code(static_cast<std::int32_t>(google::rpc::Code::DEADLINE_EXCEEDED));
      SPDLOG_DEBUG("No new messages from {}", context->remote_address);
      break;
    }

    case ResponseCode::PullRetryImmediately: {
      status->set_code(static_cast<std::int32_t>(google::rpc::Code::DEADLINE_EXCEEDED));
      SPDLOG_DEBUG("Server asks to retry immediately. Server={}", context->remote_address);
      break;
    }

    case ResponseCode::PullOffsetMoved: {
      status->set_code(static_cast<std::int32_t>(google::rpc::Code::OUT_OF_RANGE));
      SPDLOG_WARN("Offset-Moved. Server={}", context->remote_address);
      break;
    }

    case ResponseCode::SubscriptionFormatError: {
      status->set_code(static_cast<std::int32_t>(google::rpc::Code::INVALID_ARGUMENT));
      SPDLOG_WARN("Server failed to parse subscription. Server={}, Remark={}", context->remote_address,
                  command.remark());
      break;
    }

    case ResponseCode::SubscriptionAbsent: {
      status->set_code(static_cast<std::int32_t>(google::rpc::Code::FAILED_PRECONDITION));
      SPDLOG_WARN("Server does not have subscription. Need prior heartbeat. Server={}, Remark={}",
                  context->remote_address, command.remark());
      break;
    }

    case ResponseCode::SubscriptionNotLatest: {
      status->set_code(static_cast<std::int32_t>(google::rpc::Code::FAILED_PRECONDITION));
      SPDLOG_WARN("Subscription version is not the latest. Server={}, Remark={}", context->remote_address,
                  command.remark());
      break;
    }

    default: {
      status->set_code(static_cast<std::int32_t>(google::rpc::Code::UNKNOWN));
      SPDLOG_WARN("Unknown response code from {}, code={}, remark={}", context->remote_address, command.code(),
                  command.remark());
      break;
    }
  }
  context->onCompletion(true);
}

void RpcClientRemoting::asyncSend(const SendMessageRequest& request,
                                  InvocationContext<SendMessageResponse>* invocation_context) {
  assert(invocation_context);

  // Assign RequestCode
  invocation_context->request_code = RequestCode::SendMessage;
  invocation_context->request = absl::make_unique<SendMessageRequest>();
  invocation_context->request->CopyFrom(request);

  auto header = new SendMessageRequestHeader();

  // Assign topic
  assert(request.has_message());
  const auto& message = request.message();
  const auto& topic = message.topic();
  if (topic.resource_namespace().empty()) {
    header->topic(topic.name());
  } else {
    header->topic(absl::StrJoin({topic.resource_namespace(), topic.name()}, "%"));
  }

  // Assign queue-id
  if (request.has_partition()) {
    header->queueId(request.partition().id());
  }

  absl::flat_hash_map<std::string, std::string> properties;
  properties.insert(message.user_attribute().begin(), message.user_attribute().end());

  auto keys = message.system_attribute().keys();
  if (!keys.empty()) {
    std::string all_keys = absl::StrJoin(keys.begin(), keys.end(), RemotingConstants::KeySeparator);
    properties.insert({RemotingConstants::Keys, all_keys});
  }

  if (!message.system_attribute().tag().empty()) {
    properties.insert({RemotingConstants::Tags, message.system_attribute().tag()});
  }

  properties.insert({RemotingConstants::MessageId, message.system_attribute().message_id()});

  switch (message.system_attribute().timed_delivery_case()) {
    case rmq::SystemAttribute::kDeliveryTimestamp: {
      if (message.system_attribute().has_delivery_timestamp()) {
        auto timestamp = message.system_attribute().delivery_timestamp();
        timeval tv{};
        tv.tv_sec = timestamp.seconds();
        tv.tv_usec = timestamp.nanos() / 1000;

        properties.insert({RemotingConstants::StartDeliveryTime,
                           std::to_string(absl::ToInt64Milliseconds(absl::DurationFromTimeval(tv)))});
        break;
      }
    }
    case rmq::SystemAttribute::kDelayLevel: {
      properties.insert({RemotingConstants::DelayLevel, std::to_string(message.system_attribute().delay_level())});
      break;
    }
    default: {
      break;
    }
  }

  if (!message.system_attribute().message_group().empty()) {
    properties.insert({RemotingConstants::MessageGroup, message.system_attribute().message_group()});
  }

  std::uint32_t system_flag = 0;
  if (rmq::MessageType::TRANSACTION == message.system_attribute().message_type()) {
    system_flag |= RemotingConstants::FlagTransactionPrepare;
  }

  if (rmq::Encoding::GZIP == message.system_attribute().body_encoding()) {
    system_flag |= RemotingConstants::FlagCompression;
  }

  header->systemFlag(static_cast<std::int32_t>(system_flag));

  RemotingCommand command = RemotingCommand::createRequest(RequestCode::SendMessage, header);

  // Copy body to remoting command
  auto& body = command.mutableBody();
  std::size_t body_length = request.message().body().length();
  body.resize(body_length);
  memcpy(body.data(), request.message().body().data(), body_length);

  write(std::move(command), invocation_context);
}

void RpcClientRemoting::asyncQueryAssignment(const QueryAssignmentRequest& request,
                                             InvocationContext<QueryAssignmentResponse>* invocation_context) {
  QueryRouteRequest query_route_request;
  query_route_request.mutable_topic()->CopyFrom(request.topic());
  auto context = new InvocationContext<QueryRouteResponse>();
  context->callback = [invocation_context, request](const InvocationContext<QueryRouteResponse>* ctx) {
    invocation_context->status = ctx->status;
    invocation_context->response.mutable_common()->CopyFrom(ctx->response.common());
    if (ctx->status.ok() && google::rpc::Code::OK == ctx->response.common().status().code()) {
      for (const auto& partition : ctx->response.partitions()) {
        auto assignment = new rmq::Assignment();
        assignment->mutable_partition()->CopyFrom(partition);
        invocation_context->response.mutable_assignments()->AddAllocated(assignment);
      }
    } else {
      SPDLOG_WARN("Failed to query route for topic: {}", request.topic().name());
    }
    invocation_context->onCompletion(true);
  };
  asyncQueryRoute(query_route_request, context);
}

void RpcClientRemoting::asyncReceive(const ReceiveMessageRequest& request,
                                     InvocationContext<ReceiveMessageResponse>* invocation_context) {
  assert(invocation_context);

  // Assign RequestCode
  invocation_context->request_code = RequestCode::PopMessage;
  invocation_context->request = absl::make_unique<ReceiveMessageRequest>();
  invocation_context->request->CopyFrom(request);

  auto header = new PopMessageRequestHeader();
  const auto& group = request.group();
  if (group.resource_namespace().empty()) {
    header->consumerGroup(group.name());
  } else {
    header->consumerGroup(absl::StrJoin({group.resource_namespace(), group.name()}, "%"));
  }

  const auto& topic = request.partition().topic();
  if (topic.resource_namespace().empty()) {
    header->topic(topic.name());
  } else {
    header->topic(absl::StrJoin({topic.resource_namespace(), topic.name()}, "%"));
  }

  header->queueId(request.partition().id());

  header->batchSize(request.batch_size());

  timeval tv;
  tv.tv_sec = request.invisible_duration().seconds();
  tv.tv_usec = request.invisible_duration().nanos() / 1000;
  header->invisibleTime(absl::ToInt64Milliseconds(absl::DurationFromTimeval(tv)));

  if (request.has_filter_expression()) {
    switch (request.filter_expression().type()) {
      case rmq::FilterType::TAG: {
        header->expressionType(RemotingConstants::FilterTypeTag);
        if (!request.filter_expression().expression().empty()) {
          header->expression(request.filter_expression().expression());
        } else {
          header->expression("*");
        }
        break;
      }
      case rmq::FilterType::SQL: {
        header->expressionType(RemotingConstants::FilterTypeSQL);
        header->expression(request.filter_expression().expression());
        break;
      }
      default: {
        SPDLOG_WARN("Unsupported filter-type. Use defaults that accept all");
        header->expressionType(RemotingConstants::FilterTypeTag);
        header->expression("*");
        break;
      }
    }
  } else {
    header->expressionType(RemotingConstants::FilterTypeTag);
    header->expression("*");
  }

  RemotingCommand command = RemotingCommand::createRequest(RequestCode::PopMessage, header);

  write(std::move(command), invocation_context);
}

void RpcClientRemoting::asyncAck(const AckMessageRequest& request,
                                 InvocationContext<AckMessageResponse>* invocation_context) {
  // Assign RequestCode
  invocation_context->request_code = RequestCode::AckMessage;
  invocation_context->request = absl::make_unique<AckMessageRequest>();
  invocation_context->request->CopyFrom(request);

  auto header = new AckMessageRequestHeader();
  if (request.group().resource_namespace().empty()) {
    header->consumerGroup(request.group().name());
  } else {
    header->consumerGroup(absl::StrJoin({request.group().resource_namespace(), request.group().name()}, "%"));
  }

  if (request.topic().resource_namespace().empty()) {
    header->topic(request.topic().name());
  } else {
    header->topic(absl::StrJoin({request.topic().resource_namespace(), request.topic().name()}, "%"));
  }

  header->receiptHandle(request.receipt_handle());
  auto&& command = RemotingCommand::createRequest(RequestCode::AckMessage, header);
  write(std::move(command), invocation_context);
}

void RpcClientRemoting::asyncNack(const NackMessageRequest& request,
                                  InvocationContext<NackMessageResponse>* invocation_context) {
  invocation_context->onCompletion(true);
}

void RpcClientRemoting::asyncHeartbeat(const HeartbeatRequest& request,
                                       InvocationContext<HeartbeatResponse>* invocation_context) {
  // Assign RequestCode
  invocation_context->request_code = RequestCode::Heartbeat;
  invocation_context->request = absl::make_unique<HeartbeatRequest>();
  invocation_context->request->CopyFrom(request);

  HeartbeatData heartbeat_data;
  heartbeat_data.client_id_ = request.client_id();
  switch (request.client_data_case()) {
    case rmq::HeartbeatRequest::ClientDataCase::kConsumerData: {
      ConsumerData consumer_data;
      auto group = request.consumer_data().group();
      if (group.resource_namespace().empty()) {
        consumer_data.group_name_ = group.name();
      } else {
        consumer_data.group_name_ = absl::StrJoin({group.resource_namespace(), group.name()}, "%");
      }

      const auto& c_data = request.consumer_data();

      switch (c_data.consume_model()) {
        case rmq::ConsumeModel::BROADCASTING: {
          consumer_data.message_model_ = MessageModel::BROADCASTING;
          break;
        }
        case rmq::ConsumeModel::CLUSTERING: {
          consumer_data.message_model_ = MessageModel::CLUSTERING;
          break;
        }
        default: {
          break;
        }
      }

      switch (c_data.consume_policy()) {
        case rmq::ConsumePolicy::RESUME: {
          consumer_data.consume_from_where_ = ConsumeFromWhere::ConsumeFromLastOffset;
          break;
        }
        case rmq::ConsumePolicy::PLAYBACK: {
          consumer_data.consume_from_where_ = ConsumeFromWhere::ConsumeFromFirstOffset;
          break;
        }
        case rmq::ConsumePolicy::TARGET_TIMESTAMP: {
          consumer_data.consume_from_where_ = ConsumeFromWhere::ConsumeFromTimestamp;
          break;
        }
        default: {
          break;
        }
      }

      switch (c_data.consume_type()) {
        case rmq::ConsumeMessageType::PASSIVE: {
          consumer_data.consume_type_ = ConsumeType::ConsumePassively;
          break;
        }
        case rmq::ConsumeMessageType::ACTIVE: {
          consumer_data.consume_type_ = ConsumeType::ConsumeActively;
          break;
        }
        default: {
          break;
        }
      }

      for (const auto& sub : c_data.subscriptions()) {
        SubscriptionData sub_data;
        if (sub.topic().resource_namespace().empty()) {
          sub_data.topic_ = sub.topic().name();
        } else {
          sub_data.topic_ = absl::StrJoin({sub.topic().resource_namespace(), sub.topic().name()}, "%");
        }

        if (sub.has_expression()) {
          switch (sub.expression().type()) {
            case rmq::FilterType::TAG: {
              if (!sub.expression().expression().empty()) {
                sub_data.sub_string_ = sub.expression().expression();
              } else {
                sub_data.sub_string_ = "*";
              }
              break;
            }
            case rmq::FilterType::SQL: {
              break;
            }
            default: {
              break;
            }
          }
        } else {
          sub_data.sub_string_ = "*";
        }
        consumer_data.subscription_data_set_.emplace(sub_data);
      }

      heartbeat_data.consumer_data_set_.emplace(consumer_data);
      break;
    }

    case rmq::HeartbeatRequest::ClientDataCase::kProducerData: {
      break;
    }
    default: {
      break;
    }
  }

  auto command = RemotingCommand::createRequest(RequestCode::Heartbeat, nullptr);

  google::protobuf::Struct root;
  heartbeat_data.encode(root);
  std::string json;
  google::protobuf::util::MessageToJsonString(root, &json);
  command.mutableBody().resize(json.length());
  memcpy(command.mutableBody().data(), json.data(), json.length());

  write(std::move(command), invocation_context);
}

void RpcClientRemoting::asyncHealthCheck(const HealthCheckRequest& request,
                                         InvocationContext<HealthCheckResponse>* invocation_context) {
  invocation_context->onCompletion(true);
}

void RpcClientRemoting::asyncEndTransaction(const EndTransactionRequest& request,
                                            InvocationContext<EndTransactionResponse>* invocation_context) {
}

void RpcClientRemoting::asyncPollCommand(const PollCommandRequest& request,
                                         InvocationContext<PollCommandResponse>* invocation_context) {
}

void RpcClientRemoting::asyncPull(const PullMessageRequest& request,
                                  InvocationContext<PullMessageResponse>* invocation_context) {
  // Assign RequestCode
  invocation_context->request_code = RequestCode::PullMessage;
  invocation_context->request = absl::make_unique<PullMessageRequest>();
  invocation_context->request->CopyFrom(request);

  auto header = new PullMessageRequestHeader();
  const auto& group = request.group();
  if (group.resource_namespace().empty()) {
    header->consumer_group_ = group.name();
  } else {
    header->consumer_group_ = absl::StrJoin({group.resource_namespace(), group.name()}, "%");
  }

  const auto& topic = request.partition().topic();
  if (topic.resource_namespace().empty()) {
    header->topic_ = topic.name();
  } else {
    header->topic_ = absl::StrJoin({topic.resource_namespace(), topic.name()}, "%");
  }

  header->queue_id_ = request.partition().id();

  header->queue_offset_ = request.offset();

  header->max_msg_number_ = request.batch_size();

  if (request.has_filter_expression()) {
    if (!request.filter_expression().expression().empty()) {
      header->subscription_ = request.filter_expression().expression();
    } else {
      header->subscription_ = "*";
    }
  } else {
    header->subscription_ = "*";
  }
  // TODO: sub-version

  auto command = RemotingCommand::createRequest(RequestCode::PullMessage, header);
  write(std::move(command), invocation_context);
}

void RpcClientRemoting::asyncQueryOffset(const QueryOffsetRequest& request,
                                         InvocationContext<QueryOffsetResponse>* invocation_context) {
  invocation_context->onCompletion(true);
}

void RpcClientRemoting::asyncForwardMessageToDeadLetterQueue(
    const ForwardMessageToDeadLetterQueueRequest& request,
    InvocationContext<ForwardMessageToDeadLetterQueueResponse>* invocation_context) {
  invocation_context->onCompletion(true);
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

void RpcClientRemoting::handleHeartbeat(const RemotingCommand& command, BaseInvocationContext* invocation_context) {
  SPDLOG_DEBUG("Handle heartbeat response. Code: {}, remark: {}", command.code(), command.remark());
  auto context = dynamic_cast<InvocationContext<HeartbeatResponse>*>(invocation_context);
  auto response_code = static_cast<ResponseCode>(command.code());
  auto status = context->response.mutable_common()->mutable_status();
  status->set_message(command.remark());

  switch (response_code) {
    case ResponseCode::Success: {
      SPDLOG_DEBUG("Heartbeat OK.");
      break;
    }

    case ResponseCode::InternalSystemError: {
      SPDLOG_WARN("Heartbeat failed. cause: {}", command.remark());
      status->set_code(static_cast<std::int32_t>(google::rpc::Code::INTERNAL));
      break;
    }

    case ResponseCode::TooManyRequests: {
      SPDLOG_WARN("Heartbeat failed, cause: {}", command.remark());
      status->set_code(static_cast<std::int32_t>(google::rpc::Code::RESOURCE_EXHAUSTED));
      break;
    }
    default: {
      SPDLOG_WARN("Heartbeat failed, cause: {}", command.remark());
      status->set_code(static_cast<std::int32_t>(google::rpc::Code::UNIMPLEMENTED));
      break;
    }
  }
  invocation_context->onCompletion(true);
}

ROCKETMQ_NAMESPACE_END