#include <chrono>
#include <memory>
#include <system_error>

#include "RpcClient.h"
#include "absl/synchronization/mutex.h"
#include "gtest/gtest.h"

#include "ClientManagerImpl.h"
#include "rocketmq/Logger.h"
#include "rocketmq/RocketMQ.h"
#include "rocketmq/TransportType.h"

ROCKETMQ_NAMESPACE_BEGIN

class ClientManagerRemotingTest : public testing::Test {
public:
  void SetUp() override {
    Logger& logger = getLogger();
    logger.setLevel(Level::Debug);
    logger.setConsoleLevel(Level::Debug);
    logger.init();

    client_config_.transportType(TransportType::Remoting);
    client_manager_ = std::make_shared<ClientManagerImpl>(client_config_);
    client_manager_->start();
  }

  void TearDown() override {
    client_manager_.reset();
  }

protected:
  std::string group_{"CG_TestGroup"};
  std::string topic_{"zhanhui-test"};
  std::string client_id_{"Test-0"};
  std::string name_server_host_{"11.163.70.118:9876"};
  std::string broker_host_{"11.163.70.118:10911"};
  ClientConfigImpl client_config_{group_};
  std::shared_ptr<ClientManagerImpl> client_manager_;
};

TEST_F(ClientManagerRemotingTest, testLifecycle) {
}

TEST_F(ClientManagerRemotingTest, testQueryRoute) {
  std::string target_host = "11.163.70.118:9876";
  Metadata metadata;

  QueryRouteRequest request;
  request.mutable_topic()->set_name(topic_);

  bool callback_invoked = false;
  absl::Mutex callback_mtx;
  absl::CondVar callback_cv;

  auto callback = [&](const std::error_code& ec, const TopicRouteDataPtr& route) {
    callback_invoked = true;
    {
      absl::MutexLock lk(&callback_mtx);
      callback_cv.SignalAll();
    }
  };

  client_manager_->resolveRoute(target_host, metadata, request, std::chrono::seconds(3), callback);

  {
    absl::MutexLock lk(&callback_mtx);
    callback_cv.WaitWithTimeout(&callback_mtx, absl::Seconds(10));
  }
  EXPECT_TRUE(callback_invoked);
}

class TestSendCallback : public SendCallback {
public:
  TestSendCallback(bool& callback_invoked, absl::Mutex& callback_mtx, absl::CondVar& callback_cv)
      : callback_invoked_(callback_invoked), callback_mtx_(callback_mtx), callback_cv_(callback_cv) {
  }
  void onSuccess(SendResult& send_result) noexcept override {
    SPDLOG_INFO("Message sent, message-id={}", send_result.getMsgId());
    absl::MutexLock lk(&callback_mtx_);
    callback_invoked_ = true;
    callback_cv_.SignalAll();
  }

  void onFailure(const std::error_code& ec) noexcept override {
    SPDLOG_WARN("Send message failed. Reason: {}", ec.message());
    absl::MutexLock lk(&callback_mtx_);
    callback_invoked_ = true;
    callback_cv_.SignalAll();
  }

private:
  bool& callback_invoked_;
  absl::Mutex& callback_mtx_;
  absl::CondVar& callback_cv_;
};

TEST_F(ClientManagerRemotingTest, testSend) {
  std::string target_host = "11.163.70.118:10911";
  Metadata metadata;
  SendMessageRequest request;
  request.mutable_partition()->set_id(1);
  std::string body = "test";
  request.mutable_message()->set_body(body);
  request.mutable_message()->mutable_topic()->set_name(topic_);
  auto system_attribute = request.mutable_message()->mutable_system_attribute();
  system_attribute->set_message_id("unique_key");
  system_attribute->set_tag("tagA");

  bool callback_invoked = false;
  absl::Mutex callback_mtx;
  absl::CondVar callback_cv;

  auto send_callback = new TestSendCallback(callback_invoked, callback_mtx, callback_cv);

  client_manager_->send(target_host, metadata, request, send_callback);

  {
    absl::MutexLock lk(&callback_mtx);
    callback_cv.WaitWithTimeout(&callback_mtx, absl::Seconds(3));
  }
  EXPECT_TRUE(callback_invoked);
}

TEST_F(ClientManagerRemotingTest, testAssignment) {
  Metadata metadata;
  QueryAssignmentRequest request;
  request.mutable_topic()->set_name(topic_);
  request.mutable_group()->set_name(group_);

  absl::Mutex callback_mtx;
  absl::CondVar callback_cv;
  bool callback_invoked = false;
  auto callback = [&](const std::error_code& ec, const QueryAssignmentResponse& response) {
    SPDLOG_DEBUG("QueryAssignmentResponse: {}", response.DebugString());
    callback_invoked = true;
    {
      absl::MutexLock lk(&callback_mtx);
      callback_cv.SignalAll();
    }
  };
  client_manager_->addNameServer(name_server_host_);
  client_manager_->queryAssignment(broker_host_, metadata, request, std::chrono::seconds(3), callback);

  {
    absl::MutexLock lk(&callback_mtx);
    if (!callback_invoked) {
      callback_cv.WaitWithTimeout(&callback_mtx, absl::Seconds(3));
    }
  }
  EXPECT_TRUE(callback_invoked);
}

class TestReceiveMessageCallback : public ReceiveMessageCallback {
public:
  TestReceiveMessageCallback(bool& callback_invoked, absl::Mutex& callback_mtx, absl::CondVar& callback_cv,
                             std::vector<std::string>& receipt_handles)
      : callback_invoked_(callback_invoked), callback_mtx_(callback_mtx), callback_cv_(callback_cv),
        receipt_handles_(receipt_handles) {
  }

  void onCompletion(const std::error_code& ec, const ReceiveMessageResult& result) override {
    absl::MutexLock lk(&callback_mtx_);
    callback_invoked_ = true;
    for (const auto& message : result.messages) {
      receipt_handles_.push_back(message.receiptHandle());
    }
    callback_cv_.SignalAll();
  }

private:
  bool& callback_invoked_;
  absl::Mutex& callback_mtx_;
  absl::CondVar& callback_cv_;
  std::vector<std::string>& receipt_handles_;
};

TEST_F(ClientManagerRemotingTest, testPop) {
  std::string target_host = "11.163.70.118:10911";
  Metadata metadata;
  ReceiveMessageRequest request;
  request.mutable_partition()->set_id(-1);
  request.mutable_group()->set_name(group_);
  request.mutable_partition()->mutable_topic()->set_name(topic_);
  request.set_client_id(client_id_);
  request.mutable_invisible_duration()->set_seconds(10);
  request.set_batch_size(32);

  bool callback_invoked = false;
  absl::Mutex callback_mtx;
  absl::CondVar callback_cv;

  std::vector<std::string> receipt_handles;

  auto receive_callback =
      std::make_shared<TestReceiveMessageCallback>(callback_invoked, callback_mtx, callback_cv, receipt_handles);

  client_manager_->receiveMessage(target_host, metadata, request, std::chrono::seconds(3), receive_callback);
  {
    absl::MutexLock lk(&callback_mtx);
    callback_cv.WaitWithTimeout(&callback_mtx, absl::Seconds(10));
  }
  EXPECT_TRUE(callback_invoked);

  for (const auto& handle : receipt_handles) {
    AckMessageRequest request;
    request.mutable_group()->set_name(group_);
    request.mutable_topic()->set_name(topic_);
    request.set_client_id(client_id_);
    request.set_receipt_handle(handle);

    bool callback_invoked = false;
    absl::Mutex callback_mtx;
    absl::CondVar callback_cv;

    auto callback = [&](const std::error_code& ec) {
      if (ec) {
        SPDLOG_WARN("Failed to ack message");
      }

      absl::MutexLock lk(&callback_mtx);
      callback_invoked = true;
      callback_cv.SignalAll();
    };

    client_manager_->ack(target_host, metadata, request, std::chrono::seconds(3), callback);

    {
      absl::MutexLock lk(&callback_mtx);
      callback_cv.WaitWithTimeout(&callback_mtx, absl::Seconds(10));
    }
    EXPECT_TRUE(callback_invoked);
  }
}

TEST_F(ClientManagerRemotingTest, DISABLED_testAck) {
  std::string target_host = "11.163.70.118:10911";
  Metadata metadata;
  AckMessageRequest request;
  request.mutable_group()->set_name(group_);
  request.mutable_topic()->set_name(topic_);
  request.set_client_id(client_id_);
  request.set_receipt_handle("1592 1636987395338 10000 1 1 broker-a 0 1618");

  bool callback_invoked = false;
  absl::Mutex callback_mtx;
  absl::CondVar callback_cv;

  auto callback = [&](const std::error_code& ec) {
    if (ec) {
      SPDLOG_WARN("Failed to ack message");
    }

    absl::MutexLock lk(&callback_mtx);
    callback_invoked = true;
    callback_cv.SignalAll();
  };

  client_manager_->ack(target_host, metadata, request, std::chrono::seconds(3), callback);

  {
    absl::MutexLock lk(&callback_mtx);
    callback_cv.WaitWithTimeout(&callback_mtx, absl::Seconds(10));
  }
  EXPECT_TRUE(callback_invoked);
}

ROCKETMQ_NAMESPACE_END