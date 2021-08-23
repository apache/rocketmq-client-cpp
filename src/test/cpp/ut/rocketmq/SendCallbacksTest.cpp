#include "SendCallbacks.h"
#include "gtest/gtest.h"
#include <thread>

ROCKETMQ_NAMESPACE_BEGIN

class SendCallbacksTest : public testing::Test {};

TEST_F(SendCallbacksTest, testAwait) {
  std::string msg_id("Msg-0");
  AwaitSendCallback callback;
  std::thread waker([&]() {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    SendResult send_result;
    send_result.setMsgId(msg_id);
    callback.onSuccess(send_result);
  });

  callback.await();
  EXPECT_TRUE(callback);
  EXPECT_EQ(callback.sendResult().getMsgId(), msg_id);
  if (waker.joinable()) {
    waker.join();
  }
}

ROCKETMQ_NAMESPACE_END