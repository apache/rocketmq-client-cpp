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

#include "MetricBidiReactor.h"

#include "Signature.h"

ROCKETMQ_NAMESPACE_BEGIN

MetricBidiReactor::MetricBidiReactor(std::weak_ptr<Client> client,
                                     opencensus::proto::agent::metrics::v1::MetricsService::Stub* stub)
    : client_(client), stub_(stub) {
  grpc::ClientContext context;
  auto ptr = client_.lock();

  Metadata metadata;
  Signature::sign(ptr->config(), metadata);

  for (const auto& entry : metadata) {
    context.AddMetadata(entry.first, entry.second);
  }
  context.set_deadline(std::chrono::system_clock::now() + absl::ToChronoMilliseconds(ptr->config().request_timeout));
  stub_->async()->Export(&context, this);
  StartCall();
}

ROCKETMQ_NAMESPACE_END