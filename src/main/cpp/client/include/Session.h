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
#pragma once

#include <chrono>
#include <functional>
#include <grpcpp/client_context.h>
#include <grpcpp/impl/codegen/status.h>
#include <iostream>
#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"
#include "apache/rocketmq/v1/definition.grpc.pb.h"
#include "apache/rocketmq/v1/service.grpc.pb.h"
#include "apache/rocketmq/v1/service.pb.h"
#include "grpcpp/grpcpp.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

namespace rmq = apache::rocketmq::v1;

class Session {
public:
  virtual ~Session() = default;

  virtual void queryRoute(absl::flat_hash_map<std::string, std::string> metadata, const rmq::QueryRouteRequest* request,
                          std::function<void(const grpc::Status&, const rmq::QueryRouteResponse&)> cb) PURE;

  virtual void send(absl::flat_hash_map<std::string, std::string> metadata, const rmq::SendMessageRequest* request,
                    std::function<void(const grpc::Status&, const rmq::SendMessageResponse&)> cb) PURE;

  virtual void queryAssignment(absl::flat_hash_map<std::string, std::string> metadata,
                               const rmq::QueryAssignmentRequest* request,
                               std::function<void(const grpc::Status&, const rmq::QueryAssignmentResponse&)> cb) PURE;

  virtual void receive(absl::flat_hash_map<std::string, std::string> metadata,
                       const rmq::ReceiveMessageRequest* request,
                       std::function<void(const grpc::Status&, const rmq::ReceiveMessageResponse&)> cb) PURE;

  virtual void ack(absl::flat_hash_map<std::string, std::string> metadata, const rmq::AckMessageRequest* request,
                   std::function<void(const grpc::Status&, const rmq::AckMessageResponse&)> cb) PURE;

  virtual void heartbeat(absl::flat_hash_map<std::string, std::string> metadata, const rmq::HeartbeatRequest* request,
                         std::function<void(const grpc::Status&, const rmq::HeartbeatResponse&)> cb) PURE;

  virtual void healthCheck(absl::flat_hash_map<std::string, std::string> metadata,
                           const rmq::HealthCheckRequest* request,
                           std::function<void(const grpc::Status&, const rmq::HealthCheckResponse&)> cb) PURE;

  virtual void endTransaction(absl::flat_hash_map<std::string, std::string> metadata,
                              const rmq::EndTransactionRequest* request,
                              std::function<void(const grpc::Status&, const rmq::EndTransactionResponse&)> cb) PURE;

  virtual void queryOffset(absl::flat_hash_map<std::string, std::string> metadata,
                           const rmq::QueryOffsetRequest* request,
                           std::function<void(const grpc::Status&, const rmq::QueryOffsetResponse&)> cb) PURE;

  virtual void pull(absl::flat_hash_map<std::string, std::string> metadata, const rmq::PullMessageRequest* request,
                    std::function<void(const grpc::Status&, const rmq::PullMessageResponse&)> cb) PURE;

  virtual void forwardMessageToDeadLetterQueue(
      absl::flat_hash_map<std::string, std::string> metadata,
      const rmq::ForwardMessageToDeadLetterQueueRequest* request,
      std::function<void(const grpc::Status&, const rmq::ForwardMessageToDeadLetterQueueResponse&)> cb) PURE;
};

ROCKETMQ_NAMESPACE_END