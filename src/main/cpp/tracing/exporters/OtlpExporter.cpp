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
#include "OtlpExporter.h"
#include "ClientConfigImpl.h"
#include "InvocationContext.h"
#include "MixAll.h"
#include "Signature.h"
#include "UtilAll.h"
#include "absl/strings/str_split.h"
#include "fmt/format.h"
#include "opentelemetry/proto/collector/trace/v1/trace_service.pb.h"
#include "opentelemetry/proto/common/v1/common.pb.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

ROCKETMQ_NAMESPACE_BEGIN

namespace trace = opentelemetry::proto::trace::v1;
namespace common = opentelemetry::proto::common::v1;

opencensus::trace::Sampler& Samplers::always() {
  static opencensus::trace::AlwaysSampler sampler;
  return sampler;
}

void OtlpExporter::start() {
  std::shared_ptr<OtlpExporter> self = shared_from_this();
  auto handler = absl::make_unique<OtlpExporterHandler>(self);
  handler->start();
  opencensus::trace::exporter::SpanExporter::RegisterHandler(std::move(handler));
}

void ExportClient::asyncExport(const collector_trace::ExportTraceServiceRequest& request,
                               InvocationContext<collector_trace::ExportTraceServiceResponse>* invocation_context) {
  auto completion_queue = completion_queue_.lock();
  if (!completion_queue) {
    // Server should have shutdown
    return;
  }

  invocation_context->response_reader =
      stub_->PrepareAsyncExport(&invocation_context->context, request, completion_queue.get());
  invocation_context->response_reader->StartCall();
  invocation_context->response_reader->Finish(&invocation_context->response, &invocation_context->status,
                                              invocation_context);
}

const int OtlpExporterHandler::SPAN_ID_SIZE = 8;
const int OtlpExporterHandler::TRACE_ID_SIZE = 16;

OtlpExporterHandler::OtlpExporterHandler(std::weak_ptr<OtlpExporter> exporter)
    : exporter_(std::move(exporter)), completion_queue_(std::make_shared<CompletionQueue>()) {
  auto exp = exporter_.lock();
}

void OtlpExporterHandler::start() {
  poll_thread_ = std::thread(std::bind(&OtlpExporterHandler::poll, this));
  {
    absl::MutexLock lk(&start_mtx_);
    start_cv_.Wait(&start_mtx_);
  }
}

void OtlpExporterHandler::shutdown() {
  stopped_.store(true, std::memory_order_relaxed);
  if (poll_thread_.joinable()) {
    poll_thread_.join();
  }
}

void OtlpExporterHandler::syncExportClients() {
  auto exp = exporter_.lock();
  if (!exp) {
    return;
  }

  std::vector<std::string>&& hosts = exp->hosts();
  {
    absl::MutexLock lk(&clients_map_mtx_);
    for (auto i = clients_map_.begin(); i != clients_map_.end();) {
      if (std::none_of(hosts.cbegin(), hosts.cend(), [&](const std::string& host) { return i->first == host; })) {
        clients_map_.erase(i++);
      } else {
        i++;
      }
    }

    auto client_manager = exp->clientManager().lock();
    for (const auto& host : hosts) {
      if (!clients_map_.contains(host)) {
        if (client_manager) {
          auto channel = client_manager->createChannel(host);
          auto export_client = absl::make_unique<ExportClient>(completion_queue_, channel);
          clients_map_.emplace(host, std::move(export_client));
        }
      }
    }
  }
}

void OtlpExporterHandler::poll() {
  {
    // Notify main thread that the poller thread has started
    absl::MutexLock lk(&start_mtx_);
    start_cv_.SignalAll();
  }

  while (!stopped_.load(std::memory_order_relaxed)) {
    bool ok = false;
    void* opaque_invocation_context;
    while (completion_queue_->Next(&opaque_invocation_context, &ok)) {
      auto invocation_context = static_cast<BaseInvocationContext*>(opaque_invocation_context);
      if (!ok) {
        // the call is dead
        SPDLOG_WARN("CompletionQueue#Next assigned ok false, indicating the call is dead");
      }
      invocation_context->onCompletion(ok);
    }
    SPDLOG_INFO("CompletionQueue is fully drained and shut down");
  }
  SPDLOG_INFO("poll completed and quit");
}

void OtlpExporterHandler::Export(const std::vector<::opencensus::trace::exporter::SpanData>& spans) {
  auto exp = exporter_.lock();
  if (!exp) {
    return;
  }

  switch (exp->traceMode()) {
    case TraceMode::Off:
      return;
    case TraceMode::Develop: {
      {
        for (const auto& span : spans) {
          SPDLOG_INFO("trace span {} --> {}: {}", absl::FormatTime(span.start_time()),
                      absl::FormatTime(span.end_time()), span.name().data());
          for (const auto& event : span.annotations().events()) {
            for (const auto& attr : event.event().attributes()) {
              switch (attr.second.type()) {
                case opencensus::trace::AttributeValueRef::Type::kString:
                  SPDLOG_INFO("Annotation {} attribute: {} --> {}", event.event().description().data(), attr.first,
                              attr.second.string_value());
                  break;
                case opencensus::trace::AttributeValueRef::Type::kInt:
                  SPDLOG_INFO("Annotation {} attribute: {} --> {}", event.event().description().data(), attr.first,
                              attr.second.int_value());
                  break;
                case opencensus::trace::AttributeValueRef::Type::kBool:
                  SPDLOG_INFO("Annotation {} attribute: {} --> {}", event.event().description().data(), attr.first,
                              attr.second.bool_value());
                  break;
              }
            }
          }
          SPDLOG_INFO("Attributes size={}", span.attributes().size());
          for (const auto& attribute : span.attributes()) {
            switch (attribute.second.type()) {
              case opencensus::trace::AttributeValueRef::Type::kString:
                SPDLOG_INFO("Span attribute: {} --> {}", attribute.first, attribute.second.string_value());
                break;
                break;
              case opencensus::trace::AttributeValueRef::Type::kBool:
                SPDLOG_INFO("Span attribute: {} --> {}", attribute.first, attribute.second.bool_value());
                break;
              case opencensus::trace::AttributeValueRef::Type::kInt:
                SPDLOG_INFO("Span attribute: {} --> {}", attribute.first, attribute.second.int_value());
                break;
            }
          }
        }
      }
      return;
    }
    case TraceMode::Grpc:
      break;
  }

  syncExportClients();
  absl::MutexLock lk(&clients_map_mtx_);
  if (clients_map_.empty()) {
    SPDLOG_WARN("No exporter client is available");
    return;
  }

  uint32_t client_index = round_robin_++ % clients_map_.size();

  auto iterator = clients_map_.begin();
  for (uint32_t i = 0; i < client_index; i++) {
    iterator++;
  }

  auto& exporter_client = iterator->second;

  collector_trace::ExportTraceServiceRequest request;

  auto resource = new trace::ResourceSpans();
  auto instrument_library_span = new trace::InstrumentationLibrarySpans();

  uint8_t span_id_buf[SPAN_ID_SIZE];
  uint8_t trace_id_buf[TRACE_ID_SIZE];
  for (const auto& span : spans) {
    auto item = new trace::Span();

    span.context().trace_id().CopyTo(trace_id_buf);
    item->set_trace_id(&trace_id_buf, TRACE_ID_SIZE);

    span.context().span_id().CopyTo(span_id_buf);
    item->set_span_id(&span_id_buf, SPAN_ID_SIZE);

    span.parent_span_id().CopyTo(span_id_buf);
    item->set_parent_span_id(&span_id_buf, SPAN_ID_SIZE);

    item->set_name(span.name().data());

    item->set_start_time_unix_nano(absl::ToUnixNanos(span.start_time()));
    item->set_end_time_unix_nano(absl::ToUnixNanos(span.end_time()));

    item->set_kind(trace::Span_SpanKind::Span_SpanKind_SPAN_KIND_CLIENT);

    // OpenCensus has annotations or message_events, which maps to events in OpenTelemetry.
    if (!span.message_events().events().empty()) {
      for (const auto& event : span.message_events().events()) {
        auto ev = new trace::Span::Event();
        ev->set_time_unix_nano(absl::ToUnixNanos(event.timestamp()));
      }
      item->set_dropped_events_count(span.message_events().dropped_events_count());
    }

    if (!span.annotations().events().empty()) {
      for (const auto& annotation : span.annotations().events()) {
        // Specialized annotation to adjust span start-time.
        // OpenCensus does not expose function to modify span start time. As as result, we need to apply this system
        // annotation duration transforming opencensus span to OpenTelemetry span.
        if (annotation.event().description() == MixAll::SPAN_ANNOTATION_AWAIT_CONSUMPTION) {
          for (const auto& attr : annotation.event().attributes()) {
            if (attr.first == MixAll::SPAN_ANNOTATION_ATTR_START_TIME) {
              assert(attr.second.type() == opencensus::trace::AttributeValueRef::Type::kInt);
              item->set_start_time_unix_nano(attr.second.int_value() * 1e6);
            }
          }
          continue;
        }
        if (annotation.event().description() == MixAll::SPAN_ANNOTATION_MESSAGE_KEYS) {
          for (const auto& attr : annotation.event().attributes()) {
            if (attr.first == MixAll::SPAN_ANNOTATION_MESSAGE_KEYS) {
              assert(attr.second.type() == opencensus::trace::AttributeValueRef::Type::kString);
              std::string message_keys = attr.second.string_value();
              std::vector<std::string> key_list = absl::StrSplit(message_keys, MixAll::MESSAGE_KEY_SEPARATOR);
              auto key_kv = new common::KeyValue();
              key_kv->set_key(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_KEYS);
              auto key_value = new common::AnyValue();
              auto key_array_value = new common::ArrayValue();
              for (const auto& key : key_list) {
                auto value = new common::AnyValue();
                value->set_string_value(key);
                key_array_value->mutable_values()->AddAllocated(value);
              }
              key_value->set_allocated_array_value(key_array_value);
              key_kv->set_allocated_value(key_value);
              item->mutable_attributes()->AddAllocated(key_kv);
            }
          }
          continue;
        }
        auto ev = new trace::Span::Event();
        ev->set_time_unix_nano(absl::ToUnixNanos(annotation.timestamp()));
        auto attrs = annotation.event().attributes();
        for (const auto& attr : attrs) {
          auto kv = new common::KeyValue();
          kv->set_key(attr.first);
          auto value = new common::AnyValue();
          switch (attr.second.type()) {
            case opencensus::trace::AttributeValueRef::Type::kString:
              value->set_string_value(attr.second.string_value());
              break;
            case opencensus::trace::AttributeValueRef::Type::kInt:
              value->set_int_value(attr.second.int_value());
              break;
            case opencensus::trace::AttributeValueRef::Type::kBool:
              value->set_bool_value(attr.second.bool_value());
              break;
          }
          ev->mutable_attributes()->AddAllocated(kv);
        }
        item->mutable_events()->AddAllocated(ev);
      }
      item->set_dropped_events_count(span.annotations().dropped_events_count());
    }

    for (const auto& link : span.links()) {
      auto span_link = new trace::Span::Link();

      link.trace_id().CopyTo(trace_id_buf);
      item->set_trace_id(&trace_id_buf, TRACE_ID_SIZE);

      link.span_id().CopyTo(span_id_buf);
      item->set_trace_id(&span_id_buf, SPAN_ID_SIZE);

      for (const auto& attribute : link.attributes()) {
        auto kv = new common::KeyValue();
        kv->set_key(attribute.first);
        auto value = new common::AnyValue();
        switch (attribute.second.type()) {
          case opencensus::trace::AttributeValueRef::Type::kString:
            value->set_string_value(attribute.second.string_value());
            break;
          case opencensus::trace::AttributeValueRef::Type::kBool:
            value->set_bool_value(attribute.second.bool_value());
            break;
          case opencensus::trace::AttributeValueRef::Type::kInt:
            value->set_int_value(attribute.second.int_value());
            break;
        }
        kv->set_allocated_value(value);
        span_link->mutable_attributes()->AddAllocated(kv);
      }

      item->mutable_links()->AddAllocated(span_link);
    }
    item->set_dropped_links_count(span.num_attributes_dropped());

    for (const auto& attribute : span.attributes()) {
      auto kv = new common::KeyValue();
      kv->set_key(attribute.first);
      auto value = new common::AnyValue();
      switch (attribute.second.type()) {
        case opencensus::trace::AttributeValueRef::Type::kString:
          value->set_string_value(attribute.second.string_value());
          break;
        case opencensus::trace::AttributeValueRef::Type::kBool:
          value->set_bool_value(attribute.second.bool_value());
          break;
        case opencensus::trace::AttributeValueRef::Type::kInt:
          value->set_int_value(attribute.second.int_value());
          break;
      }
      kv->set_allocated_value(value);
      item->mutable_attributes()->AddAllocated(kv);
    }

    if (span.status().ok()) {
      item->mutable_status()->set_code(trace::Status_StatusCode::Status_StatusCode_STATUS_CODE_OK);
    } else {
      item->mutable_status()->set_code(trace::Status_StatusCode::Status_StatusCode_STATUS_CODE_ERROR);
      item->mutable_status()->set_message(span.status().error_message());
    }

    instrument_library_span->mutable_instrumentation_library()->mutable_name()->assign(MixAll::OTLP_NAME_VALUE);
    instrument_library_span->mutable_instrumentation_library()->mutable_version()->assign(
        ClientConfigImpl::CLIENT_VERSION);
    instrument_library_span->mutable_spans()->AddAllocated(item);
  }
  resource->mutable_instrumentation_library_spans()->AddAllocated(instrument_library_span);

  auto telemetry_sdk_language_kv = new common::KeyValue();
  telemetry_sdk_language_kv->set_key(MixAll::TRACE_RESOURCE_ATTRIBUTE_KEY_TELEMETRY_SDK_LANGUAGE);
  auto telemetry_sdk_language_value = new common::AnyValue();
  telemetry_sdk_language_value->set_string_value(MixAll::TRACE_RESOURCE_ATTRIBUTE_VALUE_TELEMETRY_SDK_LANGUAGE);
  telemetry_sdk_language_kv->set_allocated_value(telemetry_sdk_language_value);
  resource->mutable_resource()->mutable_attributes()->AddAllocated(telemetry_sdk_language_kv);

  auto host_name_kv = new common::KeyValue();
  host_name_kv->set_key(MixAll::TRACE_RESOURCE_ATTRIBUTE_KEY_HOST_NAME);
  auto host_name_value = new common::AnyValue();
  host_name_value->set_string_value(UtilAll::hostname());
  host_name_kv->set_allocated_value(host_name_value);
  resource->mutable_resource()->mutable_attributes()->AddAllocated(host_name_kv);

  auto service_name_kv = new common::KeyValue();
  service_name_kv->set_key(MixAll::TRACE_RESOURCE_ATTRIBUTE_KEY_SERVICE_NAME);
  auto service_name_value = new common::AnyValue();
  service_name_value->set_string_value(MixAll::TRACE_RESOURCE_ATTRIBUTE_VALUE_SERVICE_NAME);
  service_name_kv->set_allocated_value(service_name_value);
  resource->mutable_resource()->mutable_attributes()->AddAllocated(service_name_kv);

  request.mutable_resource_spans()->AddAllocated(resource);

  auto invocation_context = new InvocationContext<collector_trace::ExportTraceServiceResponse>();
  invocation_context->remote_address = iterator->first;
  auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(3);
  invocation_context->context.set_deadline(deadline);

  absl::flat_hash_map<std::string, std::string> metadata;
  Signature::sign(exp->clientConfig(), metadata);
  for (const auto& entry : metadata) {
    invocation_context->context.AddMetadata(entry.first, entry.second);
  }
  auto callback = [](const InvocationContext<collector_trace::ExportTraceServiceResponse>* invocation_context) {
    if (invocation_context->status.ok()) {
      SPDLOG_DEBUG("Export tracing spans OK, target={}", invocation_context->remote_address);
    } else {
      SPDLOG_WARN("Failed to export tracing spans to {}, gRPC code:{}, gRPC error message: {}",
                  invocation_context->remote_address, invocation_context->status.error_code(),
                  invocation_context->status.error_message());
    }
  };
  invocation_context->callback = callback;

  exporter_client->asyncExport(request, invocation_context);
}

thread_local std::uint32_t OtlpExporterHandler::round_robin_ = 0;

ROCKETMQ_NAMESPACE_END