#pragma once

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "AsyncCallback.h"
#include "ConsumeType.h"
#include "CredentialsProvider.h"
#include "MQMessageExt.h"
#include "MQMessageQueue.h"

ROCKETMQ_NAMESPACE_BEGIN

class PullConsumerImpl;

class DefaultMQPullConsumer {
public:
  explicit DefaultMQPullConsumer(const std::string& group_name);

  void start();

  void shutdown();

  std::future<std::vector<MQMessageQueue>> queuesFor(const std::string& topic);

  std::future<int64_t> queryOffset(const OffsetQuery& query);

  bool pull(const PullMessageQuery& request, PullResult& pull_result);

  void pull(const PullMessageQuery& request, PullCallback* callback);

  void setResourceNamespace(const std::string& resource_namespace);

  void setNamesrvAddr(const std::string& name_srv);

  void setCredentialsProvider(std::shared_ptr<CredentialsProvider> credentials_provider);

private:
  std::shared_ptr<PullConsumerImpl> impl_;
};

ROCKETMQ_NAMESPACE_END
