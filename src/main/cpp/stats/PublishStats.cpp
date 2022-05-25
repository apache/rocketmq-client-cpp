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
#include "PublishStats.h"

#include "Tag.h"

ROCKETMQ_NAMESPACE_BEGIN

PublishStats::PublishStats()
    : success_(opencensus::stats::MeasureInt64::Register("publish_success", "Number of message published", "1")),
      failure_(opencensus::stats::MeasureInt64::Register("pubish_failure", "Number of publish failures", "1")),
      latency_(opencensus::stats::MeasureInt64::Register("publish_latency", "Publish latency in milliseconds", "ms")),
      tx_commit_success_(
          opencensus::stats::MeasureInt64::Register("tx_commit_success", "Number of transactions commited", "1")),
      tx_commit_failure_(opencensus::stats::MeasureInt64::Register(
          "tx_commit_failure", "Number of failures when committing transactions", "1")),
      tx_rollback_success_(
          opencensus::stats::MeasureInt64::Register("tx_rollback_success", "Number of transactions rolled back", "1")),
      tx_rollback_failure_(opencensus::stats::MeasureInt64::Register(
          "tx_rollback_failure", "Number of failures when rolling back transactions", "1")) {
  opencensus::stats::ViewDescriptor()
      .set_name("rocketmq_send_success_total")
      .set_description("Number of messages published")
      .set_measure("publish_success")
      .set_aggregation(opencensus::stats::Aggregation::Sum())
      .add_column(Tag::topicTag())
      .add_column(Tag::clientIdTag())
      .RegisterForExport();

  opencensus::stats::ViewDescriptor()
      .set_name("rocketmq_send_failure_total")
      .set_description("Number of publish failures")
      .set_measure("pubish_failure")
      .set_aggregation(opencensus::stats::Aggregation::Sum())
      .add_column(Tag::topicTag())
      .add_column(Tag::clientIdTag())
      .RegisterForExport();

  opencensus::stats::ViewDescriptor()
      .set_name("rocketmq_send_cost_time")
      .set_description("Publish latency")
      .set_measure("publish_latency")
      .set_aggregation(opencensus::stats::Aggregation::Distribution(
          opencensus::stats::BucketBoundaries::Explicit({5, 10, 20, 50, 500})))
      .add_column(Tag::topicTag())
      .add_column(Tag::clientIdTag())
      .RegisterForExport();

  opencensus::stats::ViewDescriptor()
      .set_name("rocketmq_commit_success_total")
      .set_description("Number of transactions committed")
      .set_measure("tx_commit_success")
      .set_aggregation(opencensus::stats::Aggregation::Sum())
      .add_column(Tag::topicTag())
      .add_column(Tag::clientIdTag())
      .RegisterForExport();

  opencensus::stats::ViewDescriptor()
      .set_name("rocketmq_commit_failure_total")
      .set_description("Number of failures when committing transactions")
      .set_measure("tx_commit_failure")
      .set_aggregation(opencensus::stats::Aggregation::Sum())
      .add_column(Tag::topicTag())
      .add_column(Tag::clientIdTag())
      .RegisterForExport();

  opencensus::stats::ViewDescriptor()
      .set_name("rocketmq_rollback_success_total")
      .set_description("Number of transactions rolled back")
      .set_measure("tx_rollback_success")
      .set_aggregation(opencensus::stats::Aggregation::Sum())
      .add_column(Tag::topicTag())
      .add_column(Tag::clientIdTag())
      .RegisterForExport();

  opencensus::stats::ViewDescriptor()
      .set_name("rocketmq_rollback_failure_total")
      .set_description("Number of failures when rolling back transactions")
      .set_measure("tx_rollback_failure")
      .set_aggregation(opencensus::stats::Aggregation::Sum())
      .add_column(Tag::topicTag())
      .add_column(Tag::clientIdTag())
      .RegisterForExport();
}

ROCKETMQ_NAMESPACE_END