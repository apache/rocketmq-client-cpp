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

#include "Client.h"
#include "Exporter.h"
#include "grpcpp/grpcpp.h"
#include "opencensus/proto/agent/metrics/v1/metrics_service.grpc.pb.h"

ROCKETMQ_NAMESPACE_BEGIN

class OpencensusExporter : public Exporter {
public:
  OpencensusExporter(std::shared_ptr<grpc::Channel> channel, std::weak_ptr<Client> client);

  void exportMetrics(
      const std::vector<std::pair<opencensus::stats::ViewDescriptor, opencensus::stats::ViewData>>& data) override;

private:
  std::weak_ptr<Client> client_;
  std::unique_ptr<opencensus::proto::agent::metrics::v1::MetricsService::Stub> stub_;
};

ROCKETMQ_NAMESPACE_END