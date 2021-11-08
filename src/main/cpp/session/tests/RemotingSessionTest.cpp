#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <system_error>
#include <thread>

#include "gtest/gtest.h"

#include "QueryRouteRequestHeader.h"
#include "RemotingCommand.h"
#include "RemotingSession.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class RemotingSessionTest : public testing::Test {
public:
  void SetUp() override {
    context_ = std::make_shared<asio::io_context>();
    work_guard_ =
        std::make_shared<asio::executor_work_guard<asio::io_context::executor_type>>(context_->get_executor());

    loop_ = std::thread([this]() {
      {
        absl::MutexLock lk(&start_mtx_);
        started_ = true;
        start_cv_.SignalAll();
      }

      while (started_) {
        asio::error_code ec;
        context_->run(ec);
        if (ec) {
          std::cerr << ec.message() << std::endl;
        }
      }

      std::cout << "io_context::run() exit" << std::endl;
    });

    {
      absl::MutexLock lk(&start_mtx_);
      start_cv_.WaitWithTimeout(&start_mtx_, absl::Milliseconds(100));
    }
  }

  void TearDown() override {
    started_ = false;
    context_->stop();

    if (loop_.joinable()) {
      loop_.join();
    }
  }

protected:
  std::shared_ptr<asio::io_context> context_;
  std::shared_ptr<asio::executor_work_guard<asio::io_context::executor_type>> work_guard_;

  std::thread loop_;
  bool started_{false};
  absl::Mutex start_mtx_;
  absl::CondVar start_cv_;
  std::string endpoint_{"100.81.180.83:9876"};

  std::string topic_{"zhanhui-test"};
};

TEST_F(RemotingSessionTest, DISABLED_testConnect) {
  auto session = std::make_shared<RemotingSession>(context_, endpoint_);
  session->connect(std::chrono::milliseconds(1000));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  ASSERT_EQ(SessionState::Connected, session->state());
}

TEST_F(RemotingSessionTest, DISABLED_testWrite) {
  auto session = std::make_shared<RemotingSession>(context_, endpoint_);
  session->connect(std::chrono::milliseconds(1000));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  ASSERT_EQ(SessionState::Connected, session->state());

  auto header = new QueryRouteRequestHeader();
  header->topic(topic_);
  RemotingCommand command = RemotingCommand::createRequest(RequestCode::QueryRoute, header);

  auto&& frame = command.encode();
  std::error_code ec;
  session->write(frame, ec);
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  if (ec) {
    std::cerr << ec.message() << std::endl;
  }
}

ROCKETMQ_NAMESPACE_END