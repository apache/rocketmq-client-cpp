#include "TopicPublishInfo.h"
#include "TopicRouteData.h"

#include "LoggerImpl.h"
#include "MixAll.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_join.h"

ROCKETMQ_NAMESPACE_BEGIN

thread_local uint32_t TopicPublishInfo::send_which_queue_ = MixAll::random(0, 100);

TopicPublishInfo::TopicPublishInfo(absl::string_view topic, TopicRouteDataPtr topic_route_data)
    : topic_(topic.data(), topic.length()), topic_route_data_(std::move(topic_route_data)) {
  updatePublishInfo();
}

bool TopicPublishInfo::selectOneMessageQueue(MQMessageQueue& message_queue) {
  unsigned int index = ++send_which_queue_;
  {
    absl::MutexLock lock(&partition_list_mtx_);
    if (partition_list_.empty()) {
      return false;
    }
    auto& partition = partition_list_[index % (partition_list_.size())];
    message_queue = partition.asMessageQueue();
  }
  return true;
}

bool TopicPublishInfo::selectOneActiveMessageQueue(absl::flat_hash_set<std::string>& isolated,
                                                   MQMessageQueue& message_queue) {
  unsigned int index = ++send_which_queue_;
  {
    absl::MutexLock lock(&partition_list_mtx_);
    if (partition_list_.empty()) {
      SPDLOG_DEBUG("message queue list is empty");
      return false;
    }

    for (std::vector<MQMessageQueue>::size_type i = 0; i < partition_list_.size(); ++i) {
      message_queue = partition_list_[index++ % (partition_list_.size())].asMessageQueue();
      if (!isolated.contains(message_queue.serviceAddress())) {
        SPDLOG_DEBUG("Selected host={}", message_queue.serviceAddress());
        return true;
      }
    }
  }
  return false;
}

bool TopicPublishInfo::takeMessageQueues(absl::flat_hash_set<std::string>& isolated,
                                         std::vector<MQMessageQueue>& candidates, uint32_t count) {

  unsigned int index = ++send_which_queue_;
  {
    absl::MutexLock lock(&partition_list_mtx_);
    if (partition_list_.empty()) {
      SPDLOG_DEBUG("message queue list empty");
      return false;
    }

    for (std::vector<MQMessageQueue>::size_type i = 0; i < partition_list_.size(); ++i) {
      const MQMessageQueue& message_queue = partition_list_[index++ % (partition_list_.size())].asMessageQueue();
      if (!isolated.contains(message_queue.serviceAddress())) {
        auto search = std::find_if(candidates.begin(), candidates.end(), [&](const MQMessageQueue& item) {
          return item.getBrokerName() == message_queue.getBrokerName();
        });
        if (std::end(candidates) == search) {
          candidates.emplace_back(message_queue);
        }
        if (candidates.size() >= count) {
          return true;
        }
      }
    }
  }
  return !candidates.empty();
}

void TopicPublishInfo::updatePublishInfo() {

  std::vector<Partition> writable_partition_list;
  {
    for (const auto& partition : topic_route_data_->partitions()) {
      assert(partition.topic().name() == topic_);
      if (Permission::READ == partition.permission() || Permission::NONE == partition.permission()) {
        continue;
      }
      if (MixAll::MASTER_BROKER_ID != partition.broker().id()) {
        continue;
      }
      writable_partition_list.push_back(partition);
    }
  }

  if (writable_partition_list.empty()) {
    SPDLOG_WARN("No writable partition is current available. Skip updating publish table for topic={}", topic_);
    return;
  }

  {
    absl::MutexLock lk(&partition_list_mtx_);
    partition_list_.swap(writable_partition_list);
  }
}

void TopicPublishInfo::topicRouteData(TopicRouteDataPtr topic_route_data) {

  SPDLOG_DEBUG("Update publish info according to renewed route data of topic={}", topic_);
  { topic_route_data_ = std::move(topic_route_data); }
  updatePublishInfo();
}

std::vector<MQMessageQueue> TopicPublishInfo::getMessageQueueList() {
  std::vector<MQMessageQueue> message_queue_list;
  {
    absl::MutexLock lock(&partition_list_mtx_);
    for (const auto& partition : partition_list_) {
      if (Permission::READ == partition.permission() || Permission::NONE == partition.permission()) {
        continue;
      }

      MQMessageQueue message_queue(partition.asMessageQueue());
      if (message_queue.serviceAddress().empty()) {
        SPDLOG_WARN("Failed to resolve service address for {}", message_queue.simpleName());
        continue;
      }
      message_queue_list.emplace_back(message_queue);
    }

    std::sort(message_queue_list.begin(), message_queue_list.end());

    return message_queue_list;
  }
}

ROCKETMQ_NAMESPACE_END