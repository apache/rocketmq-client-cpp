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

#include "OpencensusExporter.h"

#include "MetricBidiReactor.h"

ROCKETMQ_NAMESPACE_BEGIN

OpencensusExporter::OpencensusExporter(std::string endpoints, std::weak_ptr<Client> client) : client_(client) {
}

void OpencensusExporter::wrap(
    const std::vector<std::pair<opencensus::stats::ViewDescriptor, opencensus::stats::ViewData>>& data,
    ExportMetricsServiceRequest& request) {
}

void OpencensusExporter::exportMetrics(
    const std::vector<std::pair<opencensus::stats::ViewDescriptor, opencensus::stats::ViewData>>& data) {
  opencensus::proto::agent::metrics::v1::ExportMetricsServiceRequest request;
  wrap(data, request);
  std::weak_ptr<OpencensusExporter> exporter{shared_from_this()};
  if (!bidi_reactor_) {
    bidi_reactor_ = absl::make_unique<MetricBidiReactor>(client_, exporter);
  }
  bidi_reactor_->write(request);
}

void OpencensusExporter::resetStream() {
  std::weak_ptr<OpencensusExporter> exporter{shared_from_this()};
  bidi_reactor_.reset(new MetricBidiReactor(client_, exporter));
}

ROCKETMQ_NAMESPACE_END
