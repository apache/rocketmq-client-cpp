#pragma once

#include <string>
#include <memory>
#include <vector>
#include <future>

#include "AsyncCallback.h"
#include "ConsumeType.h"
#include "CredentialsProvider.h"
#include "MQMessageExt.h"
#include "MQMessageQueue.h"
#include <chrono>

ROCKETMQ_NAMESPACE_BEGIN

class DefaultMQPullConsumerImpl;

class DefaultMQPullConsumer {
public:
  explicit DefaultMQPullConsumer(const std::string& group_name);

  void start();

  void shutdown();

  std::future<std::vector<MQMessageQueue>> queuesFor(const std::string& topic);

  std::future<int64_t> queryOffset(const OffsetQuery& query);

  bool pull(const PullMessageQuery& request, PullResult& pull_result);

  void pull(const PullMessageQuery& request, PullCallback* callback);

  void setArn(const std::string& arn);

  void setNamesrvAddr(const std::string& name_srv);

  void setCredentialsProvider(std::shared_ptr<CredentialsProvider> credentials_provider);

private:
  std::shared_ptr<DefaultMQPullConsumerImpl> impl_;
};


ROCKETMQ_NAMESPACE_END

