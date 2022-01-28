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
                          std::function<void(const grpc::Status&, const rmq::QueryRouteResponse&)> callback) PURE;
};

ROCKETMQ_NAMESPACE_END