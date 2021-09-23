#pragma once

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/synchronization/mutex.h"
#include "httplib.h"

#include "HttpClient.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class HttpClientImpl : public HttpClient {
public:
  HttpClientImpl();

  ~HttpClientImpl() override;

  void start() override;

  void shutdown() override;

  void
  get(HttpProtocol protocol, const std::string& host, std::uint16_t port, const std::string& path,
      const std::function<void(int, const std::multimap<std::string, std::string>&, const std::string&)>& cb) override;

private:
  absl::flat_hash_map<std::string, std::shared_ptr<httplib::Client>> clients_ GUARDED_BY(clients_mtx_);
  absl::Mutex clients_mtx_;
};

ROCKETMQ_NAMESPACE_END