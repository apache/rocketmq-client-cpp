#pragma once

#include "AdminServiceImpl.h"
#include "rocketmq/AdminServer.h"
#include <atomic>
#include <condition_variable>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <mutex>
#include <thread>

using grpc::Server;

ROCKETMQ_NAMESPACE_BEGIN

namespace admin {
class AdminServerImpl : public AdminServer {
public:
  AdminServerImpl()
      : port_(0), state_(State::CREATED), async_stub_(new rmq::Admin::AsyncService), service_(new AdminServiceImpl) {
  }

  bool start() override;

  bool stop() override;

  int port() const override {
    return port_;
  }

private:
  int port_;
  std::unique_ptr<Server> server_;
  std::atomic<State> state_;
  std::unique_ptr<rmq::Admin::AsyncService> async_stub_;
  std::unique_ptr<rmq::Admin::Service> service_;
  std::unique_ptr<grpc::ServerCompletionQueue> completion_queue_;
  std::thread loop_thread_;

  std::mutex loop_mtx_;
  std::condition_variable loop_cv_;
  void loop();
};
} // namespace admin

ROCKETMQ_NAMESPACE_END