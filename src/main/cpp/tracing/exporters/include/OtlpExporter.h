#pragma once

#include "ClientConfig.h"
#include "ClientManager.h"
#include "InvocationContext.h"
#include "absl/container/flat_hash_map.h"
#include "absl/memory/memory.h"
#include "absl/synchronization/mutex.h"
#include "opencensus/trace/exporter/span_data.h"
#include "opencensus/trace/exporter/span_exporter.h"
#include "opencensus/trace/sampler.h"
#include "opentelemetry/proto/collector/trace/v1/trace_service.grpc.pb.h"
#include "rocketmq/RocketMQ.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>

ROCKETMQ_NAMESPACE_BEGIN

namespace collector = opentelemetry::proto::collector;
namespace collector_trace = collector::trace::v1;

enum class TraceMode : std::uint8_t { OFF = 0, DEBUG = 1, GRPC = 2 };

class Samplers {
public:
  static opencensus::trace::Sampler& always();
};

class OtlpExporter : public std::enable_shared_from_this<OtlpExporter> {
public:
  OtlpExporter(std::weak_ptr<ClientManager> client_manager, ClientConfig* client_config)
      : client_manager_(std::move(client_manager)), client_config_(client_config) {}

  void updateHosts(std::vector<std::string> hosts) LOCKS_EXCLUDED(hosts_mtx_) {
    absl::MutexLock lk(&hosts_mtx_);
    hosts_ = std::move(hosts);
  }

  void start();

  void shutdown();

  std::vector<std::string> hosts() LOCKS_EXCLUDED(hosts_mtx_) {
    absl::MutexLock lk(&hosts_mtx_);
    return hosts_;
  }

  std::weak_ptr<ClientManager>& clientManager() { return client_manager_; }

  ClientConfig* clientConfig() { return client_config_; }

  void traceMode(TraceMode mode) { mode_ = mode; }

  TraceMode traceMode() const { return mode_; }

private:
  std::weak_ptr<ClientManager> client_manager_;
  ClientConfig* client_config_;

  std::vector<std::string> hosts_;
  absl::Mutex hosts_mtx_;

  TraceMode mode_{TraceMode::OFF};
};

class ExportClient {
public:
  ExportClient(std::shared_ptr<CompletionQueue> completion_queue, std::shared_ptr<grpc::Channel> channel)
      : completion_queue_(std::move(completion_queue)), stub_(collector_trace::TraceService::NewStub(channel)) {}

  void asyncExport(const collector_trace::ExportTraceServiceRequest& request,
                   InvocationContext<collector_trace::ExportTraceServiceResponse>* invocation_context);

private:
  std::weak_ptr<CompletionQueue> completion_queue_;
  std::unique_ptr<collector_trace::TraceService::Stub> stub_;
};

class OtlpExporterHandler : public ::opencensus::trace::exporter::SpanExporter::Handler {
public:
  OtlpExporterHandler(std::weak_ptr<OtlpExporter> exporter);

  void Export(const std::vector<::opencensus::trace::exporter::SpanData>& spans) override;

  void start();

  void shutdown();

private:
  std::weak_ptr<OtlpExporter> exporter_;
  std::shared_ptr<CompletionQueue> completion_queue_;
  std::thread poll_thread_;
  absl::Mutex start_mtx_;
  absl::CondVar start_cv_;

  absl::flat_hash_map<std::string, std::unique_ptr<ExportClient>> clients_map_ GUARDED_BY(clients_map_mtx_);
  absl::Mutex clients_map_mtx_;

  thread_local static std::uint32_t round_robin_;

  std::atomic_bool stopped_{false};

  void poll();

  void syncExportClients() LOCKS_EXCLUDED(clients_map_mtx_);
};

ROCKETMQ_NAMESPACE_END
