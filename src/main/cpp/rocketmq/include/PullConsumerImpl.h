#pragma once

#include <future>
#include <memory>

#include "absl/strings/string_view.h"

#include "ClientConfig.h"
#include "ClientImpl.h"
#include "ClientManagerImpl.h"
#include "rocketmq/ConsumeType.h"
#include "rocketmq/MQMessageQueue.h"
#include "rocketmq/MessageModel.h"

ROCKETMQ_NAMESPACE_BEGIN

class PullConsumerImpl : public ClientImpl, public std::enable_shared_from_this<PullConsumerImpl> {
public:
  explicit PullConsumerImpl(absl::string_view group_name) : ClientImpl(group_name) {}

  void start() override;

  void shutdown() override;

  std::future<std::vector<MQMessageQueue>> queuesFor(const std::string& topic);

  std::future<int64_t> queryOffset(const OffsetQuery& query);

  void pull(const PullMessageQuery& query, PullCallback* callback);

  void prepareHeartbeatData(HeartbeatRequest& request) override;

protected:
  std::shared_ptr<ClientImpl> self() override { return shared_from_this(); }

  MessageModel message_model_{MessageModel::CLUSTERING};
};

ROCKETMQ_NAMESPACE_END