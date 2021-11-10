#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
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
  std::string endpoint_{"11.163.70.118:9876"};

  std::string topic_{"zhanhui-test"};
};

TEST_F(RemotingSessionTest, testConnect) {

  auto callback = [](const std::vector<RemotingCommand>& commands) {};

  auto session = std::make_shared<RemotingSession>(context_, endpoint_, callback);
  session->connect(std::chrono::milliseconds(1000), true);
  ASSERT_EQ(SessionState::Connected, session->state());
}

TEST_F(RemotingSessionTest, testWrite) {

  std::atomic_bool callback_invoked{false};
  std::mutex callback_mtx;
  std::condition_variable callback_cv;

  auto callback = [&](const std::vector<RemotingCommand>& commands) -> void {
    for (const auto& command : commands) {
      EXPECT_EQ(0, command.code());
      std::string json(reinterpret_cast<const char*>(command.body().data()), command.body().size());
      ASSERT_FALSE(json.empty());
      google::protobuf::Struct root;
      google::protobuf::util::JsonParseOptions options;
      auto status = google::protobuf::util::JsonStringToMessage(json, &root, options);
      ASSERT_TRUE(status.ok());
    }

    {
      std::lock_guard<std::mutex> lk(callback_mtx);
      callback_invoked.store(true);
      callback_cv.notify_all();
    }
  };
  auto session = std::make_shared<RemotingSession>(context_, endpoint_, callback);
  session->connect(std::chrono::milliseconds(1000), true);
  ASSERT_EQ(SessionState::Connected, session->state());

  auto header = new QueryRouteRequestHeader();
  header->topic(topic_);
  RemotingCommand command = RemotingCommand::createRequest(RequestCode::QueryRoute, header);
  std::error_code ec;
  session->write(std::move(command), ec);
  ASSERT_FALSE(ec);

  {
    std::unique_lock<std::mutex> lk(callback_mtx);
    callback_cv.wait(lk, [&]() -> bool { return callback_invoked.load(); });
  }
}

ROCKETMQ_NAMESPACE_END