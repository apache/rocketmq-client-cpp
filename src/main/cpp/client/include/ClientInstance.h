#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "ClientCallback.h"
#include "ClientConfig.h"
#include "HeartbeatDataCallback.h"
#include "Histogram.h"
#include "Identifiable.h"
#include "InvocationContext.h"
#include "OrphanTransactionCallback.h"
#include "ReceiveMessageCallback.h"
#include "RpcClientImpl.h"
#include "Scheduler.h"
#include "SendMessageContext.h"
#include "TopAddressing.h"
#include "TopicRouteChangeCallback.h"
#include "TopicRouteData.h"
#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "src/cpp/server/thread_pool_interface.h"

#ifdef ENABLE_TRACING
#include "opentelemetry/exporters/otlp/otlp_exporter.h"
#include "opentelemetry/sdk/trace/batch_span_processor.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/trace/provider.h"
#endif

#include "rocketmq/AsyncCallback.h"
#include "rocketmq/CommunicationMode.h"
#include "rocketmq/State.h"

ROCKETMQ_NAMESPACE_BEGIN

class ClientInstance : public std::enable_shared_from_this<ClientInstance> {
public:
  explicit ClientInstance(std::string arn);

  ~ClientInstance();

  void start();

  void shutdown() LOCKS_EXCLUDED(rpc_clients_mtx_);

  static void assignLabels(Histogram& histogram);

  /**
   * Resolve route data from name server for the given topic.
   *
   * @param target_host Name server host address;
   * @param metadata Request headers;
   * @param request Query route entries request.
   * @param timeout RPC timeout.
   * @param cb Callback to execute once the request/response completes.
   */
  void resolveRoute(const std::string& target_host, const absl::flat_hash_map<std::string, std::string>& metadata,
                    const QueryRouteRequest& request, std::chrono::milliseconds timeout,
                    const std::function<void(bool, const TopicRouteDataPtr& ptr)>& cb) LOCKS_EXCLUDED(rpc_clients_mtx_);

  void doHealthCheck() LOCKS_EXCLUDED(clients_mtx_);

  /**
   * If inactive RPC clients refer to remote hosts that are absent from topic_route_table_, we need to purge them
   * immediately.
   */
  void cleanOfflineRpcClients() LOCKS_EXCLUDED(clients_mtx_, rpc_clients_mtx_);

  /**
   * Execute health-check on behalf of the client.
   */
  void healthCheck(const std::string& target_host, const absl::flat_hash_map<std::string, std::string>& metadata,
                   const HealthCheckRequest& request, std::chrono::milliseconds timeout,
                   const std::function<void(const std::string&, const InvocationContext<HealthCheckResponse>*)>& cb)
      LOCKS_EXCLUDED(rpc_clients_mtx_);

  bool send(const std::string& target_host, const absl::flat_hash_map<std::string, std::string>& metadata,
            SendMessageRequest& request, SendCallback* cb) LOCKS_EXCLUDED(rpc_clients_mtx_);

  /**
   * Get a RpcClient according to the given target hosts, which follows scheme specified
   * https://github.com/grpc/grpc/blob/master/doc/naming.md
   *
   * Note that a channel in gRPC is composted of one or more sub-channels. Every sub-channel represents a solid TCP
   * connection. gRPC supports a number of configurable load-balancing policy with "pick-first" as the default option.
   * Requests are distributed
   * @param target_host
   * @param need_heartbeat
   * @return
   */
  RpcClientSharedPtr getRpcClient(const std::string& target_host, bool need_heartbeat = true)
      LOCKS_EXCLUDED(rpc_clients_mtx_);

  static SendResult processSendResponse(const MQMessageQueue& message_queue, const SendMessageResponse& response);

  // only for test
  void addRpcClient(const std::string& target_host, const RpcClientSharedPtr& client) LOCKS_EXCLUDED(rpc_clients_mtx_);

  // Test purpose only
  void cleanRpcClients() LOCKS_EXCLUDED(rpc_clients_mtx_);

  void addClientObserver(std::weak_ptr<ClientCallback> client);

  void queryAssignment(const std::string& target, const absl::flat_hash_map<std::string, std::string>& metadata,
                       const QueryAssignmentRequest& request, std::chrono::milliseconds timeout,
                       const std::function<void(bool, const QueryAssignmentResponse&)>& cb);

  void receiveMessage(const std::string& target, const absl::flat_hash_map<std::string, std::string>& metadata,
                      const ReceiveMessageRequest& request, std::chrono::milliseconds timeout,
                      std::shared_ptr<ReceiveMessageCallback>& cb) LOCKS_EXCLUDED(rpc_clients_mtx_);

  void pullMessage(const std::string& target, absl::flat_hash_map<std::string, std::string>& metadata,
                   const PullMessageRequest& request, std::shared_ptr<ReceiveMessageCallback>& cb)
      LOCKS_EXCLUDED(rpc_clients_mtx_);

  /**
   * Translate protobuf message struct to domain model.
   *
   * @param item
   * @param message_ext
   * @return true if the translation succeeded; false if something wrong happens, including checksum verification, etc.
   */
  bool wrapMessage(const rmq::Message& item, MQMessageExt& message_ext);

  Scheduler& getScheduler();

  TopAddressing& topAddressing();

  /**
   * Ack message asynchronously.
   * @param target_host Target broker host address.
   * @param request Ack message request.
   */
  void ack(const std::string& target_host, const absl::flat_hash_map<std::string, std::string>& metadata,
           const AckMessageRequest& request, std::chrono::milliseconds timeout, const std::function<void(bool)>& cb);

  void nack(const std::string& target_host, const absl::flat_hash_map<std::string, std::string>& metadata,
            const NackMessageRequest& request, std::chrono::milliseconds timeout,
            const std::function<void(bool)>& callback);

  /**
   * End a transaction asynchronously.
   *
   * Callback conforms the following method signature:
   * void callable(bool rpc_ok, const EndTransactionResponse& response)
   *
   * if rpc_ok is false, the end transaction request may never reach the server and consequently no need to inspect the
   * response. If rpc_ok is true, response should be further inspected to determine business tier code and logic.
   * @param target_host
   * @param metadata
   * @param request
   * @param timeout
   * @param cb
   */
  void endTransaction(const std::string& target_host, const absl::flat_hash_map<std::string, std::string>& metadata,
                      const EndTransactionRequest& request, std::chrono::milliseconds timeout,
                      const std::function<void(bool, const EndTransactionResponse&)>& cb);

  void multiplexingCall(const std::string& target, const absl::flat_hash_map<std::string, std::string>& metadata,
                        const MultiplexingRequest& request, std::chrono::milliseconds timeout,
                        const std::function<void(const InvocationContext<MultiplexingResponse>*)>& cb);

  template <typename Callable>
  void queryOffset(const std::string& target_host, const absl::flat_hash_map<std::string, std::string>& metadata,
                   const QueryOffsetRequest& request, std::chrono::milliseconds timeout, const Callable& cb) {
    auto client = getRpcClient(target_host);
    if (!client) {
      SPDLOG_WARN("Failed to get/create RPC client for {}", target_host);
      return;
    }

    auto invocation_context = new InvocationContext<QueryOffsetResponse>();
    invocation_context->remote_address = target_host;
    invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);
    auto callback = [cb](const InvocationContext<QueryOffsetResponse>* invocation_context) {
      if (!invocation_context->status.ok()) {
        SPDLOG_WARN("Failed to send query offset request to {}. Reason: {}", invocation_context->remote_address,
                    invocation_context->status.error_message());
        cb(false, invocation_context->response);
        return;
      }

      if (google::rpc::Code::OK != invocation_context->response.common().status().code()) {
        SPDLOG_WARN("Server[host={}] failed to process query offset request. Reason: {}",
                    invocation_context->remote_address, invocation_context->response.common().DebugString());
        cb(false, invocation_context->response);
      }

      SPDLOG_DEBUG("Query offset from server[host={}] OK", invocation_context->remote_address);
      cb(true, invocation_context->response);
    };
    invocation_context->callback = callback;
    client->asyncQueryOffset(request, invocation_context);
  }

  void pullMessage(const std::string& target_host, const absl::flat_hash_map<std::string, std::string>& metadata,
                   const PullMessageRequest& request, std::chrono::milliseconds timeout,
                   const std::function<void(bool, const PullMessageResponse&)>& cb);

#ifdef ENABLE_TRACING
  nostd::shared_ptr<trace::Tracer> getTracer();
  void updateTraceProvider();
#endif

  void trace(bool trace) { trace_ = trace; }

  void heartbeat(const std::string& target_host, const absl::flat_hash_map<std::string, std::string>& metadata,
                 const HeartbeatRequest& request, std::chrono::milliseconds timeout,
                 const std::function<void(bool, const HeartbeatResponse&)>& cb);

private:
  void processPopResult(const grpc::ClientContext& client_context, const ReceiveMessageResponse& response,
                        ReceiveMessageResult& result, const std::string& target_host);

  void processPullResult(const grpc::ClientContext& client_context, const PullMessageResponse& response,
                         ReceiveMessageResult& result, const std::string& target_host);

  bool active();

  void doHeartbeat();

  void pollCompletionQueue();

  void logStats();

  Scheduler scheduler_;

  static const char* HEARTBEAT_TASK_NAME;
  static const char* STATS_TASK_NAME;
  static const char* HEALTH_CHECK_TASK_NAME;

  /**
   * Abstract resource namespace. Each user may have one or more instances and each each instance has an independent
   * abstract resource namespace.
   */
  std::string arn_;

  std::atomic<State> state_;

  std::vector<std::weak_ptr<ClientCallback>> clients_ GUARDED_BY(clients_mtx_);
  absl::Mutex clients_mtx_;

  absl::flat_hash_map<std::string, std::shared_ptr<RpcClient>> rpc_clients_ GUARDED_BY(rpc_clients_mtx_);
  absl::Mutex rpc_clients_mtx_; // protects rpc_clients_

  std::uintptr_t heartbeat_handle_{0};
  std::uintptr_t health_check_handle_{0};
  std::uintptr_t stats_handle_{0};

  std::shared_ptr<CompletionQueue> completion_queue_;
  std::unique_ptr<grpc::ThreadPoolInterface> callback_thread_pool_;

  std::thread completion_queue_thread_;

  Histogram latency_histogram_;

  absl::flat_hash_set<std::string> exporter_endpoint_set_ GUARDED_BY(exporter_endpoint_set_mtx_);
  absl::Mutex exporter_endpoint_set_mtx_;

  /**
   * Tenant-id. Each user shall have one unique identifier.
   */
  std::string tenant_id_;

  std::string service_name_{"MQ"};

  /**
   * TLS configuration
   */
  std::shared_ptr<grpc::experimental::TlsServerAuthorizationCheckConfig> server_authorization_check_config_;
  std::shared_ptr<grpc::experimental::CertificateProviderInterface> certificate_provider_;
  grpc::experimental::TlsChannelCredentialsOptions tls_channel_credential_options_;
  std::shared_ptr<grpc::ChannelCredentials> channel_credential_;
  grpc::ChannelArguments channel_arguments_;

  bool trace_{false};

  TopAddressing top_addressing_;
};

using ClientInstancePtr = std::shared_ptr<ClientInstance>;

ROCKETMQ_NAMESPACE_END