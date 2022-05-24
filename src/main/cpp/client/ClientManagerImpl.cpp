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
#include "ClientManagerImpl.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <memory>
#include <system_error>
#include <utility>
#include <vector>

#include "InvocationContext.h"
#include "LogInterceptor.h"
#include "LogInterceptorFactory.h"
#include "LoggerImpl.h"
#include "MessageExt.h"
#include "MetadataConstants.h"
#include "MixAll.h"
#include "Protocol.h"
#include "ReceiveMessageContext.h"
#include "RpcClient.h"
#include "RpcClientImpl.h"
#include "Scheduler.h"
#include "TlsHelper.h"
#include "UtilAll.h"
#include "google/protobuf/util/time_util.h"
#include "grpcpp/create_channel.h"
#include "rocketmq/ErrorCode.h"
#include "rocketmq/SendReceipt.h"

ROCKETMQ_NAMESPACE_BEGIN

ClientManagerImpl::ClientManagerImpl(std::string resource_namespace)
    : scheduler_(std::make_shared<SchedulerImpl>()), resource_namespace_(std::move(resource_namespace)),
      state_(State::CREATED),
      callback_thread_pool_(absl::make_unique<ThreadPoolImpl>(std::thread::hardware_concurrency())),
      latency_histogram_("Message-Latency", 11) {
  spdlog::set_level(spdlog::level::trace);
  assignLabels(latency_histogram_);

  certificate_verifier_ = grpc::experimental::ExternalCertificateVerifier::Create<InsecureCertificateVerifier>();
  tls_channel_credential_options_.set_verify_server_certs(false);
  tls_channel_credential_options_.set_check_call_host(false);
  channel_credential_ = grpc::experimental::TlsCredentials(tls_channel_credential_options_);

  // Use unlimited receive message size.
  channel_arguments_.SetMaxReceiveMessageSize(-1);

  int max_send_message_size = 1024 * 1024 * 16;
  channel_arguments_.SetMaxSendMessageSize(max_send_message_size);

  /*
   * Keep-alive settings:
   * https://github.com/grpc/grpc/blob/master/doc/keepalive.md
   * Keep-alive ping timeout duration: 3s
   * Keep-alive ping interval, 30s
   */
  channel_arguments_.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 60000);
  channel_arguments_.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 3000);
  channel_arguments_.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
  channel_arguments_.SetInt(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0);

  /*
   * If set to zero, disables retry behavior. Otherwise, transparent retries
   * are enabled for all RPCs, and configurable retries are enabled when they
   * are configured via the service config. For details, see:
   *   https://github.com/grpc/proposal/blob/master/A6-client-retries.md
   */
  channel_arguments_.SetInt(GRPC_ARG_ENABLE_RETRIES, 0);

  channel_arguments_.SetSslTargetNameOverride("localhost");

  SPDLOG_INFO("ClientManager[ResourceNamespace={}] created", resource_namespace_);
}

ClientManagerImpl::~ClientManagerImpl() {
  shutdown();
  SPDLOG_INFO("ClientManager[ResourceNamespace={}] destructed", resource_namespace_);
}

void ClientManagerImpl::start() {
  if (State::CREATED != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected client instance state: {}", state_.load(std::memory_order_relaxed));
    return;
  }
  state_.store(State::STARTING, std::memory_order_relaxed);

  callback_thread_pool_->start();

  scheduler_->start();

  std::weak_ptr<ClientManagerImpl> client_instance_weak_ptr = shared_from_this();

  auto heartbeat_functor = [client_instance_weak_ptr]() {
    auto client_instance = client_instance_weak_ptr.lock();
    if (client_instance) {
      client_instance->doHeartbeat();
    }
  };
  heartbeat_task_id_ =
      scheduler_->schedule(heartbeat_functor, HEARTBEAT_TASK_NAME, std::chrono::seconds(1), std::chrono::seconds(10));
  SPDLOG_DEBUG("Heartbeat task-id={}", heartbeat_task_id_);

  auto stats_functor_ = [client_instance_weak_ptr]() {
    auto client_instance = client_instance_weak_ptr.lock();
    if (client_instance) {
      client_instance->logStats();
    }
  };
  stats_task_id_ =
      scheduler_->schedule(stats_functor_, STATS_TASK_NAME, std::chrono::seconds(0), std::chrono::seconds(10));
  state_.store(State::STARTED, std::memory_order_relaxed);
}

void ClientManagerImpl::shutdown() {
  SPDLOG_INFO("Client manager shutdown");
  if (State::STARTED != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected client instance state: {}", state_.load(std::memory_order_relaxed));
    return;
  }
  state_.store(STOPPING, std::memory_order_relaxed);

  callback_thread_pool_->shutdown();

  if (heartbeat_task_id_) {
    scheduler_->cancel(heartbeat_task_id_);
  }

  if (stats_task_id_) {
    scheduler_->cancel(stats_task_id_);
  }

  scheduler_->shutdown();

  {
    absl::MutexLock lk(&rpc_clients_mtx_);
    rpc_clients_.clear();
    SPDLOG_DEBUG("rpc_clients_ is clear");
  }

  state_.store(State::STOPPED, std::memory_order_relaxed);
  SPDLOG_DEBUG("ClientManager stopped");
}

void ClientManagerImpl::assignLabels(Histogram& histogram) {
  histogram.labels().emplace_back("[000ms~020ms): ");
  histogram.labels().emplace_back("[020ms~040ms): ");
  histogram.labels().emplace_back("[040ms~060ms): ");
  histogram.labels().emplace_back("[060ms~080ms): ");
  histogram.labels().emplace_back("[080ms~100ms): ");
  histogram.labels().emplace_back("[100ms~120ms): ");
  histogram.labels().emplace_back("[120ms~140ms): ");
  histogram.labels().emplace_back("[140ms~160ms): ");
  histogram.labels().emplace_back("[160ms~180ms): ");
  histogram.labels().emplace_back("[180ms~200ms): ");
  histogram.labels().emplace_back("[200ms~inf): ");
}

std::vector<std::string> ClientManagerImpl::cleanOfflineRpcClients() {
  absl::flat_hash_set<std::string> hosts;
  {
    absl::MutexLock lk(&clients_mtx_);
    for (const auto& item : clients_) {
      std::shared_ptr<Client> client = item.lock();
      if (!client) {
        continue;
      }
      client->endpointsInUse(hosts);
    }
  }

  std::vector<std::string> removed;
  {
    absl::MutexLock lk(&rpc_clients_mtx_);
    for (auto it = rpc_clients_.begin(); it != rpc_clients_.end();) {
      std::string host = it->first;
      if (it->second->needHeartbeat() && !hosts.contains(host)) {
        SPDLOG_INFO("Removed RPC client whose peer is offline. RemoteHost={}", host);
        removed.push_back(host);
        rpc_clients_.erase(it++);
      } else {
        it++;
      }
    }
  }

  return removed;
}

void ClientManagerImpl::heartbeat(const std::string& target_host, const Metadata& metadata,
                                  const HeartbeatRequest& request, std::chrono::milliseconds timeout,
                                  const std::function<void(const std::error_code&, const HeartbeatResponse&)>& cb) {
  SPDLOG_DEBUG("Prepare to send heartbeat to {}. Request: {}", target_host, request.DebugString());
  auto client = getRpcClient(target_host, true);
  auto invocation_context = new InvocationContext<HeartbeatResponse>();
  invocation_context->task_name = fmt::format("Heartbeat to {}", target_host);
  invocation_context->remote_address = target_host;
  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }

  auto callback = [cb](const InvocationContext<HeartbeatResponse>* invocation_context) {
    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to send heartbeat to target_host={}. gRPC code: {}, message: {}",
                  invocation_context->remote_address, invocation_context->status.error_code(),
                  invocation_context->status.error_message());
      std::error_code ec = ErrorCode::RequestTimeout;
      cb(ec, invocation_context->response);
      return;
    }

    auto&& status = invocation_context->response.status();
    std::error_code ec;
    switch (status.code()) {
      case rmq::Code::OK: {
        cb(ec, invocation_context->response);
      } break;
      case rmq::Code::UNAUTHORIZED: {
        SPDLOG_WARN("Unauthorized: {}, host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::Unauthorized;
        cb(ec, invocation_context->response);
      } break;
      case rmq::Code::FORBIDDEN: {
        SPDLOG_WARN("Forbidden: {}, host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::Forbidden;
        cb(ec, invocation_context->response);
      } break;
      case rmq::Code::INTERNAL_SERVER_ERROR: {
        SPDLOG_WARN("InternalServerError: {}, host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::InternalServerError;
        cb(ec, invocation_context->response);
      } break;
      default: {
        SPDLOG_WARN("NotImplemented: Please upgrade SDK to latest release. Message={}, host={}", status.message(),
                    invocation_context->remote_address);
      } break;
    }
  };

  invocation_context->callback = callback;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);
  client->asyncHeartbeat(request, invocation_context);
}

void ClientManagerImpl::doHeartbeat() {
  if (State::STARTED != state_.load(std::memory_order_relaxed) &&
      State::STARTING != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected client manager state={}.", state_.load(std::memory_order_relaxed));
    return;
  }

  {
    absl::MutexLock lk(&clients_mtx_);
    for (const auto& item : clients_) {
      auto client = item.lock();
      if (client && client->active()) {
        client->heartbeat();
      }
    }
  }
}

bool ClientManagerImpl::send(const std::string& target_host, const Metadata& metadata, SendMessageRequest& request,
                             SendCallback cb) {
  assert(cb);
  SPDLOG_DEBUG("Prepare to send message to {} asynchronously", target_host);
  RpcClientSharedPtr client = getRpcClient(target_host);
  // Invocation context will be deleted in its onComplete() method.
  auto invocation_context = new InvocationContext<SendMessageResponse>();
  invocation_context->task_name = fmt::format("Send message to {}", target_host);
  invocation_context->remote_address = target_host;
  for (const auto& entry : metadata) {
    invocation_context->context.AddMetadata(entry.first, entry.second);
  }

  const std::string& topic = request.messages().begin()->topic().name();
  std::weak_ptr<ClientManager> client_manager(shared_from_this());
  auto completion_callback = [topic, cb,
                              client_manager](const InvocationContext<SendMessageResponse>* invocation_context) {
    ClientManagerPtr client_manager_ptr = client_manager.lock();
    if (!client_manager_ptr) {
      return;
    }

    if (State::STARTED != client_manager_ptr->state()) {
      // TODO: Would this leak some memroy?
      return;
    }

    SendReceipt send_receipt;
    std::error_code ec;
    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to send message to {} due to gRPC error. gRPC code: {}, gRPC error message: {}",
                  invocation_context->remote_address, invocation_context->status.error_code(),
                  invocation_context->status.error_message());
      ec = ErrorCode::RequestTimeout;
      cb(ec, send_receipt);
      return;
    }

    auto&& status = invocation_context->response.status();
    switch (invocation_context->response.status().code()) {
      case rmq::Code::OK: {
        send_receipt.message_id = invocation_context->response.entries().begin()->message_id();
        break;
      }
      case rmq::Code::TOPIC_NOT_FOUND: {
        SPDLOG_WARN("TopicNotFound: {}", status.message());
        ec = ErrorCode::NotFound;
        break;
      }
      case rmq::Code::UNAUTHORIZED: {
        SPDLOG_WARN("Unauthenticated: {}", status.message());
        ec = ErrorCode::Unauthorized;
        break;
      }
      case rmq::Code::FORBIDDEN: {
        SPDLOG_WARN("Forbidden: {}", status.message());
        ec = ErrorCode::Forbidden;
        cb(ec, send_receipt);
        break;
      }
      case rmq::Code::INTERNAL_SERVER_ERROR: {
        SPDLOG_WARN("InternalServerError: {}", status.message());
        ec = ErrorCode::InternalServerError;
        break;
      }
      default: {
        SPDLOG_WARN("Unsupported status code. Check and upgrade SDK to the latest");
        ec = ErrorCode::NotImplemented;
        break;
      }
    }
    cb(ec, send_receipt);
  };

  invocation_context->callback = completion_callback;
  client->asyncSend(request, invocation_context);
  return true;
}

/**
 * @brief Create a gRPC channel to target host.
 *
 * @param target_host
 * @return std::shared_ptr<grpc::Channel>
 */
std::shared_ptr<grpc::Channel> ClientManagerImpl::createChannel(const std::string& target_host) {
  std::vector<std::unique_ptr<grpc::experimental::ClientInterceptorFactoryInterface>> interceptor_factories;
  interceptor_factories.emplace_back(absl::make_unique<LogInterceptorFactory>());
  auto channel = grpc::experimental::CreateCustomChannelWithInterceptors(
      target_host, channel_credential_, channel_arguments_, std::move(interceptor_factories));
  return channel;
}

RpcClientSharedPtr ClientManagerImpl::getRpcClient(const std::string& target_host, bool need_heartbeat) {
  std::shared_ptr<RpcClient> client;
  {
    absl::MutexLock lock(&rpc_clients_mtx_);
    auto search = rpc_clients_.find(target_host);
    if (search == rpc_clients_.end() || !search->second->ok()) {
      if (search == rpc_clients_.end()) {
        SPDLOG_INFO("Create a RPC client to {}", target_host.data());
      } else if (!search->second->ok()) {
        SPDLOG_INFO("Prior RPC client to {} is not OK. Re-create one", target_host);
      }
      std::vector<std::unique_ptr<grpc::experimental::ClientInterceptorFactoryInterface>> interceptor_factories;
      interceptor_factories.emplace_back(absl::make_unique<LogInterceptorFactory>());
      auto channel = grpc::experimental::CreateCustomChannelWithInterceptors(
          target_host, channel_credential_, channel_arguments_, std::move(interceptor_factories));
      std::weak_ptr<ClientManager> client_manager(shared_from_this());
      client = std::make_shared<RpcClientImpl>(client_manager, channel, target_host, need_heartbeat);
      rpc_clients_.insert_or_assign(target_host, client);
    } else {
      client = search->second;
    }
  }

  if (need_heartbeat && !client->needHeartbeat()) {
    client->needHeartbeat(need_heartbeat);
  }

  return client;
}

void ClientManagerImpl::addRpcClient(const std::string& target_host, const RpcClientSharedPtr& client) {
  {
    absl::MutexLock lock(&rpc_clients_mtx_);
    rpc_clients_.insert_or_assign(target_host, client);
  }
}

void ClientManagerImpl::cleanRpcClients() {
  absl::MutexLock lk(&rpc_clients_mtx_);
  rpc_clients_.clear();
}

SendReceipt ClientManagerImpl::processSendResponse(const rmq::MessageQueue& message_queue,
                                                   const SendMessageResponse& response, std::error_code& ec) {
  SendReceipt send_receipt;

  switch (response.status().code()) {
    case rmq::Code::OK: {
      assert(response.entries_size() > 0);
      send_receipt.message_id = response.entries().begin()->message_id();
      send_receipt.transaction_id = response.entries().begin()->transaction_id();
      return send_receipt;
    }
    case rmq::Code::ILLEGAL_TOPIC: {
      ec = ErrorCode::BadRequest;
      return send_receipt;
    }
    default: {
      // TODO: handle other cases.
      break;
    }
  }
  return send_receipt;
}

void ClientManagerImpl::addClientObserver(std::weak_ptr<Client> client) {
  absl::MutexLock lk(&clients_mtx_);
  clients_.emplace_back(std::move(client));
}

void ClientManagerImpl::resolveRoute(const std::string& target_host, const Metadata& metadata,
                                     const QueryRouteRequest& request, std::chrono::milliseconds timeout,
                                     const std::function<void(const std::error_code&, const TopicRouteDataPtr&)>& cb) {
  SPDLOG_DEBUG("Name server connection URL: {}", target_host);
  SPDLOG_DEBUG("Query route request: {}", request.DebugString());
  RpcClientSharedPtr client = getRpcClient(target_host, false);
  if (!client) {
    SPDLOG_WARN("Failed to create RPC client for name server[host={}]", target_host);
    std::error_code ec = ErrorCode::RequestTimeout;
    cb(ec, nullptr);
    return;
  }

  auto invocation_context = new InvocationContext<QueryRouteResponse>();
  invocation_context->task_name = fmt::format("Query route of topic={} from {}", request.topic().name(), target_host);
  invocation_context->remote_address = target_host;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);
  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }

  auto callback = [cb](const InvocationContext<QueryRouteResponse>* invocation_context) {
    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to send query route request to server[host={}]. Reason: {}",
                  invocation_context->remote_address, invocation_context->status.error_message());
      std::error_code ec = ErrorCode::RequestTimeout;
      cb(ec, nullptr);
      return;
    }

    std::error_code ec;
    auto&& status = invocation_context->response.status();
    switch (status.code()) {
      case rmq::Code::OK: {
        std::vector<rmq::MessageQueue> message_queues;
        for (const auto& item : invocation_context->response.message_queues()) {
          message_queues.push_back(item);
        }
        auto ptr = std::make_shared<TopicRouteData>(std::move(message_queues));
        cb(ec, ptr);
      } break;
      case rmq::Code::UNAUTHORIZED: {
        SPDLOG_WARN("Unauthorized: {}. Host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::Unauthorized;
        cb(ec, nullptr);
      } break;
      case rmq::Code::FORBIDDEN: {
        SPDLOG_WARN("Forbidden: {}. Host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::Forbidden;
        cb(ec, nullptr);
      } break;
      case rmq::Code::TOPIC_NOT_FOUND: {
        SPDLOG_WARN("TopicNotFound: {}. Host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::NotFound;
        cb(ec, nullptr);
      } break;
      case rmq::Code::INTERNAL_SERVER_ERROR: {
        SPDLOG_WARN("InternalServerError: {}. Host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::InternalServerError;
        cb(ec, nullptr);
      } break;
      default: {
        SPDLOG_WARN("NotImplement: Please upgrade to latest SDK release. Host={}", invocation_context->remote_address);
        ec = ErrorCode::NotImplemented;
        cb(ec, nullptr);
      } break;
    }
  };
  invocation_context->callback = callback;
  client->asyncQueryRoute(request, invocation_context);
}

void ClientManagerImpl::queryAssignment(
    const std::string& target, const Metadata& metadata, const QueryAssignmentRequest& request,
    std::chrono::milliseconds timeout,
    const std::function<void(const std::error_code&, const QueryAssignmentResponse&)>& cb) {
  SPDLOG_DEBUG("Prepare to send query assignment request to broker[address={}]", target);
  std::shared_ptr<RpcClient> client = getRpcClient(target);

  auto callback = [&, cb](const InvocationContext<QueryAssignmentResponse>* invocation_context) {
    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to query assignment. Reason: {}", invocation_context->status.error_message());
      std::error_code ec = ErrorCode::RequestTimeout;
      cb(ec, invocation_context->response);
      return;
    }

    auto&& status = invocation_context->response.status();
    std::error_code ec;
    switch (status.code()) {
      case rmq::Code::OK: {
        SPDLOG_DEBUG("Query assignment OK");
      } break;
      case rmq::Code::UNAUTHORIZED: {
        SPDLOG_WARN("Unauthorized: {}, host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::Unauthorized;
      } break;
      case rmq::Code::FORBIDDEN: {
        SPDLOG_WARN("Forbidden: {}, host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::Forbidden;
      } break;
      case rmq::Code::INTERNAL_SERVER_ERROR: {
        SPDLOG_WARN("InternalServerError: {}, host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::InternalServerError;
      } break;
      default: {
        SPDLOG_WARN("NotImplemented: please upgrade SDK to latest release. Host={}",
                    invocation_context->remote_address);
        ec = ErrorCode::NotImplemented;
      } break;
    }
    cb(ec, invocation_context->response);
  };

  auto invocation_context = new InvocationContext<QueryAssignmentResponse>();
  invocation_context->task_name = fmt::format("QueryAssignment from {}", target);
  invocation_context->remote_address = target;
  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);
  invocation_context->callback = callback;
  client->asyncQueryAssignment(request, invocation_context);
}

void ClientManagerImpl::receiveMessage(const std::string& target_host, const Metadata& metadata,
                                       const ReceiveMessageRequest& request, std::chrono::milliseconds timeout,
                                       ReceiveMessageCallback cb) {
  SPDLOG_DEBUG("Prepare to receive message from {} asynchronously. Request: {}", target_host, request.DebugString());
  RpcClientSharedPtr client = getRpcClient(target_host);
  auto context = absl::make_unique<ReceiveMessageContext>();
  context->callback = std::move(cb);
  context->metadata = metadata;
  context->timeout = timeout;
  client->asyncReceive(request, std::move(context));
}

State ClientManagerImpl::state() const {
  return state_.load(std::memory_order_relaxed);
}

MessageConstSharedPtr ClientManagerImpl::wrapMessage(const rmq::Message& item) {
  assert(item.topic().resource_namespace() == resource_namespace_);
  auto builder = Message::newBuilder();

  // base
  builder.withTopic(item.topic().name());

  const auto& system_properties = item.system_properties();

  // Tag
  if (system_properties.has_tag()) {
    builder.withTag(system_properties.tag());
  }

  // Keys
  std::vector<std::string> keys;
  for (const auto& key : system_properties.keys()) {
    keys.push_back(key);
  }
  if (!keys.empty()) {
    builder.withKeys(std::move(keys));
  }

  // Message-Id
  const auto& message_id = system_properties.message_id();
  builder.withId(message_id);

  // Validate body digest
  const rmq::Digest& digest = system_properties.body_digest();
  bool body_digest_match = false;
  if (item.body().empty()) {
    SPDLOG_WARN("Body of message[topic={}, msgId={}] is empty", item.topic().name(),
                item.system_properties().message_id());
    body_digest_match = true;
  } else {
    switch (digest.type()) {
      case rmq::DigestType::CRC32: {
        std::string checksum;
        bool success = MixAll::crc32(item.body(), checksum);
        if (success) {
          body_digest_match = (digest.checksum() == checksum);
          if (body_digest_match) {
            SPDLOG_DEBUG("Message body CRC32 checksum validation passed.");
          } else {
            SPDLOG_WARN("Body CRC32 checksum validation failed. Actual: {}, expect: {}", checksum, digest.checksum());
          }
        } else {
          SPDLOG_WARN("Failed to calculate CRC32 checksum. Skip.");
        }
        break;
      }
      case rmq::DigestType::MD5: {
        std::string checksum;
        bool success = MixAll::md5(item.body(), checksum);
        if (success) {
          body_digest_match = (digest.checksum() == checksum);
          if (body_digest_match) {
            SPDLOG_DEBUG("Body of message[{}] MD5 checksum validation passed.", message_id);
          } else {
            SPDLOG_WARN("Body of message[{}] MD5 checksum validation failed. Expect: {}, Actual: {}", message_id,
                        digest.checksum(), checksum);
          }
        } else {
          SPDLOG_WARN("Failed to calculate MD5 digest. Skip.");
          body_digest_match = true;
        }
        break;
      }
      case rmq::DigestType::SHA1: {
        std::string checksum;
        bool success = MixAll::sha1(item.body(), checksum);
        if (success) {
          body_digest_match = (checksum == digest.checksum());
          if (body_digest_match) {
            SPDLOG_DEBUG("Body of message[{}] SHA1 checksum validation passed", message_id);
          } else {
            SPDLOG_WARN("Body of message[{}] SHA1 checksum validation failed. Expect: {}, Actual: {}", message_id,
                        digest.checksum(), checksum);
          }
        } else {
          SPDLOG_WARN("Failed to calculate SHA1 digest for message[{}]. Skip.", message_id);
        }
        break;
      }
      default: {
        SPDLOG_WARN("Unsupported message body digest algorithm");
        body_digest_match = true;
        break;
      }
    }
  }

  if (!body_digest_match) {
    SPDLOG_WARN("Message body checksum failed. MsgId={}", system_properties.message_id());
    // TODO: NACK it immediately
    return nullptr;
  }

  // Body encoding
  switch (system_properties.body_encoding()) {
    case rmq::Encoding::GZIP: {
      std::string uncompressed;
      UtilAll::uncompress(item.body(), uncompressed);
      builder.withBody(uncompressed);
      break;
    }
    case rmq::Encoding::IDENTITY: {
      builder.withBody(item.body());
      break;
    }
    default: {
      SPDLOG_WARN("Unsupported encoding algorithm");
      break;
    }
  }

  // User-properties
  std::unordered_map<std::string, std::string> properties;
  for (const auto& it : item.user_properties()) {
    properties.insert(std::make_pair(it.first, it.second));
  }
  if (!properties.empty()) {
    builder.withProperties(properties);
  }

  // Born-timestamp
  if (system_properties.has_born_timestamp()) {
    auto born_timestamp = google::protobuf::util::TimeUtil::TimestampToMilliseconds(system_properties.born_timestamp());
    builder.withBornTime(absl::ToChronoTime(absl::FromUnixMillis(born_timestamp)));
  }

  // Born-host
  builder.withBornHost(system_properties.born_host());

  // Trace-context
  if (system_properties.has_trace_context()) {
    builder.withTraceContext(system_properties.trace_context());
  }

  auto message = builder.build();

  const Message* raw = message.release();
  Message* msg = const_cast<Message*>(raw);
  Extension& extension = msg->mutableExtension();

  // Receipt-handle
  extension.receipt_handle = system_properties.receipt_handle();

  // Store-timestamp
  if (system_properties.has_store_timestamp()) {
    auto store_timestamp =
        google::protobuf::util::TimeUtil::TimestampToMilliseconds(system_properties.store_timestamp());
    extension.store_time = absl::ToChronoTime(absl::FromUnixMillis(store_timestamp));
  }

  // Store-host
  extension.store_host = system_properties.store_host();

  // Process one-of: delivery-timestamp and delay-level.
  if (system_properties.has_delivery_timestamp()) {
    auto delivery_timestamp_ms =
        google::protobuf::util::TimeUtil::TimestampToMilliseconds(system_properties.delivery_timestamp());
    extension.delivery_timepoint = absl::ToChronoTime(absl::FromUnixMillis(delivery_timestamp_ms));
  }

  // Queue-id
  extension.queue_id = system_properties.queue_id();

  // Queue-offset
  extension.offset = system_properties.queue_offset();

  // Invisible-period
  if (system_properties.has_invisible_duration()) {
    auto invisible_period = std::chrono::seconds(system_properties.invisible_duration().seconds()) +
                            std::chrono::nanoseconds(system_properties.invisible_duration().nanos());
    extension.invisible_period = invisible_period;
  }

  // Delivery attempt
  extension.delivery_attempt = system_properties.delivery_attempt();

  // Decoded Time-Point
  extension.decode_time = std::chrono::system_clock::now();

  // Extension
  {
    auto elapsed = MixAll::millisecondsOf(std::chrono::system_clock::now() - extension.store_time);
    if (elapsed >= 0) {
      latency_histogram_.countIn(elapsed / 20);
    }
  }
  return MessageConstSharedPtr(raw);
}

SchedulerSharedPtr ClientManagerImpl::getScheduler() {
  return scheduler_;
}

void ClientManagerImpl::ack(const std::string& target, const Metadata& metadata, const AckMessageRequest& request,
                            std::chrono::milliseconds timeout, const std::function<void(const std::error_code&)>& cb) {
  std::string target_host(target.data(), target.length());
  SPDLOG_DEBUG("Prepare to ack message against {} asynchronously. AckMessageRequest: {}", target_host,
               request.DebugString());
  RpcClientSharedPtr client = getRpcClient(target_host);

  auto invocation_context = new InvocationContext<AckMessageResponse>();
  invocation_context->task_name = fmt::format("Ack messages against {}", target);
  invocation_context->remote_address = target_host;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);

  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }

  // TODO: Use capture by move and pass-by-value paradigm when C++ 14 is available.
  auto callback = [request, cb](const InvocationContext<AckMessageResponse>* invocation_context) {
    std::error_code ec;
    if (!invocation_context->status.ok()) {
      ec = ErrorCode::RequestTimeout;
      cb(ec);
      return;
    }

    auto&& status = invocation_context->response.status();
    switch (status.code()) {
      case rmq::Code::OK: {
        SPDLOG_DEBUG("Ack OK. host={}", invocation_context->remote_address);
      } break;
      case rmq::Code::UNAUTHORIZED: {
        SPDLOG_WARN("Unauthorized: {}, host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::Unauthorized;
      } break;
      case rmq::Code::FORBIDDEN: {
        SPDLOG_WARN("PermissionDenied: {}, host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::Forbidden;
      } break;
      case rmq::Code::INTERNAL_SERVER_ERROR: {
        SPDLOG_WARN("InternalServerError: {}, host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::InternalServerError;
      } break;
      default: {
        SPDLOG_WARN("NotImplement: please upgrade SDK to latest release. host={}", invocation_context->remote_address);
        ec = ErrorCode::NotImplemented;
      } break;
    }
    cb(ec);
  };
  invocation_context->callback = callback;
  client->asyncAck(request, invocation_context);
}

void ClientManagerImpl::changeInvisibleDuration(
    const std::string& target_host, const Metadata& metadata, const ChangeInvisibleDurationRequest& request,
    std::chrono::milliseconds timeout, const std::function<void(const std::error_code&)>& completion_callback) {
  RpcClientSharedPtr client = getRpcClient(target_host);
  assert(client);
  auto invocation_context = new InvocationContext<ChangeInvisibleDurationResponse>();
  invocation_context->task_name = fmt::format("ChangeInvisibleDuration Message[receipt-handle={}] against {}",
                                              request.receipt_handle(), target_host);
  invocation_context->remote_address = target_host;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);

  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }

  auto callback = [completion_callback](const InvocationContext<ChangeInvisibleDurationResponse>* invocation_context) {
    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to write Nack request to wire. gRPC-code: {}, gRPC-message: {}",
                  invocation_context->status.error_code(), invocation_context->status.error_message());
      std::error_code ec = ErrorCode::RequestTimeout;
      completion_callback(ec);
      return;
    }

    std::error_code ec;
    auto&& status = invocation_context->response.status();
    switch (status.code()) {
      case rmq::Code::OK: {
        SPDLOG_DEBUG("Nack to {} OK", invocation_context->remote_address);
        break;
      };
      case rmq::Code::UNAUTHORIZED: {
        SPDLOG_WARN("Unauthorized: {}, host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::Unauthorized;
        break;
      }
      case rmq::Code::FORBIDDEN: {
        SPDLOG_WARN("Forbidden: {}, host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::Forbidden;
        break;
      }
      case rmq::Code::INTERNAL_SERVER_ERROR: {
        SPDLOG_WARN("InternalServerError: {}, host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::InternalServerError;
        break;
      }
      default: {
        SPDLOG_WARN("NotImplemented: Please upgrade to latest SDK, host={}", invocation_context->remote_address);
        ec = ErrorCode::NotImplemented;
        break;
      }
    }
    completion_callback(ec);
  };
  invocation_context->callback = callback;
  client->asyncChangeInvisibleDuration(request, invocation_context);
}

void ClientManagerImpl::endTransaction(
    const std::string& target_host, const Metadata& metadata, const EndTransactionRequest& request,
    std::chrono::milliseconds timeout,
    const std::function<void(const std::error_code&, const EndTransactionResponse&)>& cb) {
  RpcClientSharedPtr client = getRpcClient(target_host);
  if (!client) {
    SPDLOG_WARN("No RPC client for {}", target_host);
    EndTransactionResponse response;
    std::error_code ec = ErrorCode::BadRequest;
    cb(ec, response);
    return;
  }

  SPDLOG_DEBUG("Prepare to endTransaction. TargetHost={}, Request: {}", target_host.data(), request.DebugString());

  auto invocation_context = new InvocationContext<EndTransactionResponse>();
  invocation_context->task_name = fmt::format("End transaction[{}] of message[] against {}", request.transaction_id(),
                                              request.message_id(), target_host);
  invocation_context->remote_address = target_host;
  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }

  // Set RPC deadline.
  auto deadline = std::chrono::system_clock::now() + timeout;
  invocation_context->context.set_deadline(deadline);

  auto callback = [target_host, cb](const InvocationContext<EndTransactionResponse>* invocation_context) {
    std::error_code ec;
    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to write EndTransaction to wire. gRPC-code: {}, gRPC-message: {}, host={}",
                  invocation_context->status.error_code(), invocation_context->status.error_message(),
                  invocation_context->remote_address);
      ec = ErrorCode::BadRequest;
      cb(ec, invocation_context->response);
      return;
    }

    auto&& status = invocation_context->response.status();
    switch (status.code()) {
      case rmq::Code::OK: {
        SPDLOG_DEBUG("endTransaction completed OK. Response: {}, host={}", invocation_context->response.DebugString(),
                     invocation_context->remote_address);
      } break;
      case rmq::Code::UNAUTHORIZED: {
        SPDLOG_WARN("Unauthorized: {}, host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::Unauthorized;
      } break;
      case rmq::Code::FORBIDDEN: {
        SPDLOG_WARN("Forbidden: {}, host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::Forbidden;
      } break;
      case rmq::Code::INTERNAL_SERVER_ERROR: {
        SPDLOG_WARN("InternalServerError: {}, host={}", status.message(), invocation_context->remote_address);
        ec = ErrorCode::InternalServerError;
      } break;
      default: {
        SPDLOG_WARN("NotImplemented: please upgrade SDK to latest release. {}, host={}", status.message(),
                    invocation_context->remote_address);
        ec = ErrorCode::NotImplemented;
      }
    }
    cb(ec, invocation_context->response);
  };

  invocation_context->callback = callback;
  client->asyncEndTransaction(request, invocation_context);
}

void ClientManagerImpl::forwardMessageToDeadLetterQueue(const std::string& target_host,
                                                        const Metadata& metadata,
                                                        const ForwardMessageToDeadLetterQueueRequest& request,
                                                        std::chrono::milliseconds timeout,
                                                        const std::function<void(const std::error_code&)>& cb) {
  SPDLOG_DEBUG("ForwardMessageToDeadLetterQueue Request: {}", request.DebugString());
  auto client = getRpcClient(target_host);
  auto invocation_context = new InvocationContext<ForwardMessageToDeadLetterQueueResponse>();
  invocation_context->task_name =
      fmt::format("Forward message[{}] to DLQ against {}", request.message_id(), target_host);
  invocation_context->remote_address = target_host;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);

  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }

  auto callback = [cb](const InvocationContext<ForwardMessageToDeadLetterQueueResponse>* invocation_context) {
    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to transmit SendMessageToDeadLetterQueueRequest to host={}",
                  invocation_context->remote_address);
      std::error_code ec = ErrorCode::BadRequest;
      cb(ec);
      return;
    }

    SPDLOG_DEBUG("Received forwardToDeadLetterQueue response from server[host={}]", invocation_context->remote_address);
    std::error_code ec;
    switch (invocation_context->response.status().code()) {
      case rmq::Code::OK: {
        break;
      }
      case rmq::Code::INTERNAL_SERVER_ERROR: {
        ec = ErrorCode::ServiceUnavailable;
      }
      case rmq::Code::TOO_MANY_REQUESTS: {
        ec = ErrorCode::TooManyRequest;
      }
      default: {
        ec = ErrorCode::NotImplemented;
      }
    }
    cb(ec);
  };
  invocation_context->callback = callback;
  client->asyncForwardMessageToDeadLetterQueue(request, invocation_context);
}

std::error_code ClientManagerImpl::notifyClientTermination(const std::string& target_host, const Metadata& metadata,
                                                           const NotifyClientTerminationRequest& request,
                                                           std::chrono::milliseconds timeout) {
  std::error_code ec;
  auto client = getRpcClient(target_host);
  if (!client) {
    SPDLOG_WARN("Failed to create RpcClient for host={}", target_host);
    ec = ErrorCode::RequestTimeout;
    return ec;
  }

  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + timeout);
  for (const auto& item : metadata) {
    context.AddMetadata(item.first, item.second);
  }

  SPDLOG_DEBUG("NotifyClientTermination request: {}", request.DebugString());

  NotifyClientTerminationResponse response;
  {
    grpc::Status status = client->notifyClientTermination(&context, request, &response);
    if (!status.ok()) {
      SPDLOG_WARN("NotifyClientTermination failed. gRPC-code={}, gRPC-message={}, host={}", status.error_code(),
                  status.error_message(), target_host);
      ec = ErrorCode::RequestTimeout;
      return ec;
    }
  }

  auto&& status = response.status();

  switch (status.code()) {
    case rmq::Code::OK: {
      SPDLOG_DEBUG("NotifyClientTermination OK. host={}", target_host);
      break;
    }
    case rmq::Code::INTERNAL_SERVER_ERROR: {
      SPDLOG_WARN("InternalServerError: Cause={}, host={}", status.message(), target_host);
      ec = ErrorCode::InternalServerError;
      break;
    }
    case rmq::Code::UNAUTHORIZED: {
      SPDLOG_WARN("Unauthorized due to lack of valid authentication credentials: Cause={}, host={}", status.message(),
                  target_host);
      ec = ErrorCode::Unauthorized;
      break;
    }
    case rmq::Code::FORBIDDEN: {
      SPDLOG_WARN("Forbidden due to insufficient permission to the resource: Cause={}, host={}", status.message(),
                  target_host);
      ec = ErrorCode::Forbidden;
      break;
    }
    default: {
      SPDLOG_WARN("NotImplemented. Please upgrade to latest SDK release. host={}", target_host);
      ec = ErrorCode::NotImplemented;
      break;
    }
  }
  return ec;
}

void ClientManagerImpl::logStats() {
  std::string stats;
  latency_histogram_.reportAndReset(stats);
  SPDLOG_INFO("{}", stats);
}

void ClientManagerImpl::submit(std::function<void()> task) {
  State current_state = state();
  if (current_state == State::STOPPING || current_state == State::STOPPED) {
    return;
  }
  callback_thread_pool_->submit(task);
}

const char* ClientManagerImpl::HEARTBEAT_TASK_NAME = "heartbeat-task";
const char* ClientManagerImpl::STATS_TASK_NAME = "stats-task";

ROCKETMQ_NAMESPACE_END