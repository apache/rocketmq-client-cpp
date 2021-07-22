#include "rocketmq/RocketMQ.h"

#include "HttpClient.h"
#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"
#include "src/core/lib/http/httpcli.h"
#include "src/core/lib/iomgr/iomgr.h"
#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <thread>
#include <vector>

ROCKETMQ_NAMESPACE_BEGIN

struct HttpInvocationContext {
  HttpInvocationContext() { memset(&request, 0, sizeof(request)); }
  std::string host;
  std::string path;
  grpc_httpcli_request request;
  grpc_http_response response;
  std::function<void(int, const absl::flat_hash_map<std::string, std::string>&, const std::string&)> callback;
};

class GHttpClient : public HttpClient {
public:
  GHttpClient();

  ~GHttpClient() override;

  void start() override;

  void shutdown() override;

  void get(HttpProtocol protocol, const std::string& host, std::uint16_t port, const std::string& path,
           const std::function<void(int, const absl::flat_hash_map<std::string, std::string>&, const std::string&)>& cb)
      override;

  static const int STATUS_OK;

private:
  static void onCompletion(void* arg, grpc_error_handle error);

  static void destroyPollingEntity(void* arg, grpc_error_handle error);

  void poll();

  void submit0() LOCKS_EXCLUDED(pending_requests_mtx_);

  grpc_httpcli_context http_context_;
  grpc_polling_entity http_polling_entity_;
  gpr_mu* http_mtx_{nullptr};

  std::thread loop_;
  grpc_pollset_worker* worker_;
  grpc_closure destroy_;
  std::atomic_bool shutdown_;

  std::vector<HttpInvocationContext*> pending_requests_ GUARDED_BY(pending_requests_mtx_);
  absl::Mutex pending_requests_mtx_;

  static const int64_t POLL_INTERVAL;
};

ROCKETMQ_NAMESPACE_END