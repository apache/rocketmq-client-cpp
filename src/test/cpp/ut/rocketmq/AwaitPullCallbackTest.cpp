#include "AwaitPullCallback.h"
#include "MessageAccessor.h"
#include "rocketmq/MQClientException.h"
#include "rocketmq/MQMessageExt.h"
#include "rocketmq/PullResult.h"
#include "gtest/gtest.h"
#include <thread>

ROCKETMQ_NAMESPACE_BEGIN

TEST(AwaitPullCallbackTest, testOnSuccess) {
  std::vector<MQMessageExt> messages;
  PullResult pull_result(0, 32, 32, messages);
  AwaitPullCallback callback(pull_result);
  EXPECT_FALSE(callback.isCompleted());

  std::string topic{"Test"};
  int total = 32;
  int min = 32;
  int max = 128;
  int next = 64;
  auto task = [&]() {
    std::vector<MQMessageExt> msgs;
    for (int i = 0; i < total; i++) {
      MQMessageExt msg;
      msg.setTopic(topic);
      MessageAccessor::setQueueOffset(msg, i);
      msgs.emplace_back(msg);
    }
    PullResult result(32, 128, 64, msgs);
    callback.onSuccess(result);
  };

  std::thread t(task);

  callback.await();
  EXPECT_TRUE(callback.isCompleted());
  EXPECT_EQ(pull_result.messages().size(), total);
  EXPECT_EQ(pull_result.min(), min);
  EXPECT_EQ(pull_result.max(), max);
  EXPECT_EQ(pull_result.next(), next);

  if (t.joinable()) {
    t.join();
  }
}

TEST(AwaitPullCallbackTest, testOnFailure) {
  std::vector<MQMessageExt> messages;
  PullResult pull_result(0, 32, 32, messages);
  AwaitPullCallback callback(pull_result);
  std::string topic{"Test"};
  auto task = [&]() {
    MQClientException e("test exception", -1, __FILE__, __LINE__);
    callback.onException(e);
  };

  std::thread t(task);

  callback.await();
  EXPECT_TRUE(callback.isCompleted());
  EXPECT_TRUE(callback.hasFailure());
  if (t.joinable()) {
    t.join();
  }
}

ROCKETMQ_NAMESPACE_END