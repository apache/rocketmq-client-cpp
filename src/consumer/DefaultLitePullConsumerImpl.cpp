/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "DefaultLitePullConsumerImpl.h"

#ifndef WIN32
#include <signal.h>
#endif

#include "AssignedMessageQueue.hpp"
#include "FilterAPI.hpp"
#include "MQAdminImpl.h"
#include "MQClientAPIImpl.h"
#include "MQClientInstance.h"
#include "NamespaceUtil.h"
#include "LocalFileOffsetStore.h"
#include "PullAPIWrapper.h"
#include "PullSysFlag.h"
#include "RebalanceLitePullImpl.h"
#include "RemoteBrokerOffsetStore.h"
#include "UtilAll.h"
#include "Validators.h"

static const long PULL_TIME_DELAY_MILLS_WHEN_PAUSE = 1000;
static const long PULL_TIME_DELAY_MILLS_WHEN_FLOW_CONTROL = 50;

namespace rocketmq {

class DefaultLitePullConsumerImpl::MessageQueueListenerImpl : public MessageQueueListener {
 public:
  MessageQueueListenerImpl(DefaultLitePullConsumerImplPtr pull_consumer) : default_lite_pull_consumer_(pull_consumer) {}

  ~MessageQueueListenerImpl() = default;

  void messageQueueChanged(const std::string& topic,
                           std::vector<MQMessageQueue>& mq_all,
                           std::vector<MQMessageQueue>& mq_divided) override {
    auto consumer = default_lite_pull_consumer_.lock();
    if (nullptr == consumer) {
      return;
    }
    switch (consumer->messageModel()) {
      case BROADCASTING:
        consumer->updateAssignedMessageQueue(topic, mq_all);
        consumer->updatePullTask(topic, mq_all);
        break;
      case CLUSTERING:
        consumer->updateAssignedMessageQueue(topic, mq_divided);
        consumer->updatePullTask(topic, mq_divided);
        break;
      default:
        break;
    }
  }

 private:
  std::weak_ptr<DefaultLitePullConsumerImpl> default_lite_pull_consumer_;
};

class DefaultLitePullConsumerImpl::ConsumeRequest {
 public:
  ConsumeRequest(std::vector<MessageExtPtr>&& message_exts,
                 const MQMessageQueue& message_queue,
                 ProcessQueuePtr process_queue)
      : message_exts_(std::move(message_exts)), message_queue_(message_queue), process_queue_(process_queue) {}

 public:
  std::vector<MessageExtPtr>& message_exts() { return message_exts_; }

  MQMessageQueue& message_queue() { return message_queue_; }

  ProcessQueuePtr process_queue() { return process_queue_; }

 private:
  std::vector<MessageExtPtr> message_exts_;
  MQMessageQueue message_queue_;
  ProcessQueuePtr process_queue_;
};

class DefaultLitePullConsumerImpl::PullTaskImpl : public std::enable_shared_from_this<PullTaskImpl> {
 public:
  PullTaskImpl(DefaultLitePullConsumerImplPtr pull_consumer, const MQMessageQueue& message_queue)
      : default_lite_pull_consumer_(pull_consumer), message_queue_(message_queue), cancelled_(false) {}

  void run() {
    auto consumer = default_lite_pull_consumer_.lock();
    if (nullptr == consumer) {
      LOG_WARN_NEW("PullTaskImpl::run: DefaultLitePullConsumerImpl is released.");
      return;
    }

    if (cancelled_) {
      return;
    }

    if (consumer->assigned_message_queue_->isPaused(message_queue_)) {
      consumer->scheduled_thread_pool_executor_.schedule(
          std::bind(&DefaultLitePullConsumerImpl::PullTaskImpl::run, shared_from_this()),
          PULL_TIME_DELAY_MILLS_WHEN_PAUSE, time_unit::milliseconds);
      LOG_DEBUG_NEW("Message Queue: {} has been paused!", message_queue_.toString());
      return;
    }

    auto process_queue = consumer->assigned_message_queue_->getProcessQueue(message_queue_);
    if (nullptr == process_queue || process_queue->dropped()) {
      LOG_INFO_NEW("The message queue not be able to poll, because it's dropped. group={}, messageQueue={}",
                   consumer->groupName(), message_queue_.toString());
      return;
    }

    auto config = consumer->getDefaultLitePullConsumerConfig();

    if (consumer->consume_request_cache_.size() * config->pull_batch_size() > config->pull_threshold_for_all()) {
      consumer->scheduled_thread_pool_executor_.schedule(
          std::bind(&DefaultLitePullConsumerImpl::PullTaskImpl::run, shared_from_this()),
          PULL_TIME_DELAY_MILLS_WHEN_FLOW_CONTROL, time_unit::milliseconds);
      if ((consumer->consume_request_flow_control_times_++ % 1000) == 0)
        LOG_WARN_NEW(
            "The consume request count exceeds threshold {}, so do flow control, consume request count={}, "
            "flowControlTimes={}",
            config->pull_threshold_for_all(), consumer->consume_request_cache_.size(),
            consumer->consume_request_flow_control_times_);
      return;
    }

    auto cached_message_count = process_queue->getCacheMsgCount();
    if (cached_message_count > config->pull_threshold_for_queue()) {
      consumer->scheduled_thread_pool_executor_.schedule(
          std::bind(&DefaultLitePullConsumerImpl::PullTaskImpl::run, shared_from_this()),
          PULL_TIME_DELAY_MILLS_WHEN_FLOW_CONTROL, time_unit::milliseconds);
      if ((consumer->queue_flow_control_times_++ % 1000) == 0) {
        LOG_WARN_NEW(
            "The cached message count exceeds the threshold {}, so do flow control, minOffset={}, maxOffset={}, "
            "count={}, size={} MiB, flowControlTimes={}",
            config->pull_threshold_for_queue(), process_queue->getCacheMinOffset(), process_queue->getCacheMaxOffset(),
            cached_message_count, "unknown", consumer->queue_flow_control_times_);
      }
      return;
    }

    // long cachedMessageSizeInMiB = processQueue->getMsgSize() / (1024 * 1024);
    // if (cachedMessageSizeInMiB > consumer.getPullThresholdSizeForQueue()) {
    //   scheduledThreadPoolExecutor.schedule(this, PULL_TIME_DELAY_MILLS_WHEN_FLOW_CONTROL, TimeUnit.MILLISECONDS);
    //   if ((queueFlowControlTimes++ % 1000) == 0) {
    //     log.warn(
    //         "The cached message size exceeds the threshold {} MiB, so do flow control, minOffset={}, maxOffset={},
    //         "
    //         "count={}, size={} MiB, flowControlTimes={}",
    //         consumer.getPullThresholdSizeForQueue(), processQueue.getMsgTreeMap().firstKey(),
    //         processQueue.getMsgTreeMap().lastKey(), cachedMessageCount, cachedMessageSizeInMiB,
    //         queueFlowControlTimes);
    //   }
    //   return;
    // }

    // if (processQueue.getMaxSpan() > consumer.getConsumeMaxSpan()) {
    //   scheduledThreadPoolExecutor.schedule(this, PULL_TIME_DELAY_MILLS_WHEN_FLOW_CONTROL, TimeUnit.MILLISECONDS);
    //   if ((queueMaxSpanFlowControlTimes++ % 1000) == 0) {
    //     log.warn(
    //         "The queue's messages, span too long, so do flow control, minOffset={}, maxOffset={}, maxSpan={}, "
    //         "flowControlTimes={}",
    //         processQueue.getMsgTreeMap().firstKey(), processQueue.getMsgTreeMap().lastKey(),
    //         processQueue.getMaxSpan(),
    //         queueMaxSpanFlowControlTimes);
    //   }
    //   return;
    // }

    auto offset = consumer->nextPullOffset(message_queue_);
    long pull_delay_time_millis = 0;
    SubscriptionData* subscription_data = nullptr;
    try {
      if (consumer->subscription_type_ == SubscriptionType::SUBSCRIBE) {
        subscription_data = consumer->rebalance_impl_->getSubscriptionData(message_queue_.topic());
      } else {
        subscription_data = FilterAPI::buildSubscriptionData(message_queue_.topic(), SUB_ALL);
      }

      std::unique_ptr<PullResult> pull_result(
          consumer->pull(message_queue_, subscription_data, offset, config->pull_batch_size()));

      switch (pull_result->pull_status()) {
        case PullStatus::FOUND: {
          auto objLock = consumer->message_queue_lock_.fetchLockObject(message_queue_);
          std::lock_guard<std::mutex> lock(*objLock);
          if (!pull_result->msg_found_list().empty() &&
              consumer->assigned_message_queue_->getSeekOffset(message_queue_) == -1) {
            process_queue->putMessage(pull_result->msg_found_list());
            consumer->submitConsumeRequest(
                new ConsumeRequest(std::move(pull_result->msg_found_list()), message_queue_, process_queue));
          }
        } break;
        case PullStatus::OFFSET_ILLEGAL:
          LOG_WARN_NEW("The pull request offset illegal, {}", pull_result->toString());
          break;
        case PullStatus::NO_NEW_MSG:
        case PullStatus::NO_MATCHED_MSG:
          pull_delay_time_millis = 1000;
          break;
        case PullStatus::NO_LATEST_MSG:
          pull_delay_time_millis = config->pull_time_delay_millis_when_exception();
          break;
        default:
          break;
      }

      consumer->updatePullOffset(message_queue_, pull_result->next_begin_offset());
    } catch (std::exception& e) {
      pull_delay_time_millis = config->pull_time_delay_millis_when_exception();
      LOG_ERROR_NEW("An error occurred in pull message process. {}", e.what());
    }

    if (consumer->subscription_type_ != SubscriptionType::SUBSCRIBE) {
      delete subscription_data;
    }

    if (!cancelled_) {
      consumer->scheduled_thread_pool_executor_.schedule(
          std::bind(&DefaultLitePullConsumerImpl::PullTaskImpl::run, shared_from_this()), pull_delay_time_millis,
          time_unit::milliseconds);
    } else {
      LOG_WARN_NEW("The Pull Task is cancelled after doPullTask, {}", message_queue_.toString());
    }
  }

 public:
  inline const MQMessageQueue& message_queue() { return message_queue_; }

  inline bool is_cancelled() const { return cancelled_; }
  inline void set_cancelled(bool cancelled) { cancelled_ = cancelled; }

 private:
  std::weak_ptr<DefaultLitePullConsumerImpl> default_lite_pull_consumer_;
  MQMessageQueue message_queue_;
  volatile bool cancelled_;
};

DefaultLitePullConsumerImpl::DefaultLitePullConsumerImpl(DefaultLitePullConsumerConfigPtr config)
    : DefaultLitePullConsumerImpl(config, nullptr) {}

DefaultLitePullConsumerImpl::DefaultLitePullConsumerImpl(DefaultLitePullConsumerConfigPtr config, RPCHookPtr rpcHook)
    : MQClientImpl(config, rpcHook),
      start_time_(UtilAll::currentTimeMillis()),
      subscription_type_(SubscriptionType::NONE),
      consume_request_flow_control_times_(0),
      queue_flow_control_times_(0),
      next_auto_commit_deadline_(-1LL),
      auto_commit_(true),
      message_queue_listener_(nullptr),
      assigned_message_queue_(new AssignedMessageQueue()),
      scheduled_thread_pool_executor_("PullMsgThread", config->pull_thread_nums(), false),
      scheduled_executor_service_("MonitorMessageQueueChangeThread", false),
      rebalance_impl_(new RebalanceLitePullImpl(this)),
      pull_api_wrapper_(nullptr),
      offset_store_(nullptr) {}

DefaultLitePullConsumerImpl::~DefaultLitePullConsumerImpl() = default;

void DefaultLitePullConsumerImpl::start() {
#ifndef WIN32
  /* Ignore the SIGPIPE */
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  ::sigaction(SIGPIPE, &sa, 0);
#endif

  switch (service_state_) {
    case CREATE_JUST: {
      // wrap namespace
      client_config_->set_group_name(
          NamespaceUtil::wrapNamespace(client_config_->name_space(), client_config_->group_name()));

      LOG_INFO_NEW("the consumer [{}] start beginning.", client_config_->group_name());

      service_state_ = START_FAILED;

      checkConfig();

      if (messageModel() == MessageModel::CLUSTERING) {
        client_config_->changeInstanceNameToPID();
      }

      // init client_instance_
      MQClientImpl::start();

      // init rebalance_impl_
      rebalance_impl_->set_consumer_group(client_config_->group_name());
      rebalance_impl_->set_message_model(getDefaultLitePullConsumerConfig()->message_model());
      rebalance_impl_->set_allocate_mq_strategy(getDefaultLitePullConsumerConfig()->allocate_mq_strategy());
      rebalance_impl_->set_client_instance(client_instance_.get());

      // init pull_api_wrapper_
      pull_api_wrapper_.reset(new PullAPIWrapper(client_instance_.get(), client_config_->group_name()));
      // TODO: registerFilterMessageHook

      // init offset_store_
      switch (getDefaultLitePullConsumerConfig()->message_model()) {
        case MessageModel::BROADCASTING:
          offset_store_.reset(new LocalFileOffsetStore(client_instance_.get(), client_config_->group_name()));
          break;
        case MessageModel::CLUSTERING:
          offset_store_.reset(new RemoteBrokerOffsetStore(client_instance_.get(), client_config_->group_name()));
          break;
      }
      offset_store_->load();

      scheduled_thread_pool_executor_.set_thread_nums(getDefaultLitePullConsumerConfig()->pull_thread_nums());
      scheduled_thread_pool_executor_.startup();
      scheduled_executor_service_.startup();

      // register consumer
      bool registerOK = client_instance_->registerConsumer(client_config_->group_name(), this);
      if (!registerOK) {
        service_state_ = CREATE_JUST;
        THROW_MQEXCEPTION(MQClientException, "The cousumer group[" + client_config_->group_name() +
                                                 "] has been created before, specify another name please.",
                          -1);
      }

      client_instance_->start();

      startScheduleTask();

      LOG_INFO_NEW("the consumer [{}] start OK", client_config_->group_name());
      service_state_ = RUNNING;

      operateAfterRunning();
      break;
    }
    case RUNNING:
    case START_FAILED:
    case SHUTDOWN_ALREADY:
      THROW_MQEXCEPTION(MQClientException, "The PullConsumer service state not OK, maybe started once", -1);
      break;
    default:
      break;
  };
}

void DefaultLitePullConsumerImpl::checkConfig() {
  const auto& groupname = client_config_->group_name();

  // check consumerGroup
  Validators::checkGroup(groupname);

  // consumerGroup
  if (DEFAULT_CONSUMER_GROUP == groupname) {
    THROW_MQEXCEPTION(MQClientException,
                      "consumerGroup can not equal " + DEFAULT_CONSUMER_GROUP + ", please specify another one.", -1);
  }

  // messageModel
  if (getDefaultLitePullConsumerConfig()->message_model() != BROADCASTING &&
      getDefaultLitePullConsumerConfig()->message_model() != CLUSTERING) {
    THROW_MQEXCEPTION(MQClientException, "messageModel is valid", -1);
  }

  // allocateMessageQueueStrategy
  if (nullptr == getDefaultLitePullConsumerConfig()->allocate_mq_strategy()) {
    THROW_MQEXCEPTION(MQClientException, "allocateMessageQueueStrategy is null", -1);
  }

  // if (getDefaultLitePullConsumerConfig()->getConsumerTimeoutMillisWhenSuspend() <
  //     getDefaultLitePullConsumerConfig()->getBrokerSuspendMaxTimeMillis()) {
  //   THROW_MQEXCEPTION(
  //       MQClientException,
  //       "Long polling mode, the consumer consumerTimeoutMillisWhenSuspend must greater than
  //       brokerSuspendMaxTimeMillis",
  //       -1);
  // }
}

void DefaultLitePullConsumerImpl::startScheduleTask() {
  scheduled_executor_service_.schedule(
      std::bind(&DefaultLitePullConsumerImpl::fetchTopicMessageQueuesAndComparePeriodically, this), 1000 * 10,
      time_unit::milliseconds);
}

void DefaultLitePullConsumerImpl::fetchTopicMessageQueuesAndComparePeriodically() {
  try {
    fetchTopicMessageQueuesAndCompare();
  } catch (std::exception& e) {
    LOG_ERROR_NEW("ScheduledTask fetchMessageQueuesAndCompare exception: {}", e.what());
  }

  // next round
  scheduled_executor_service_.schedule(
      std::bind(&DefaultLitePullConsumerImpl::fetchTopicMessageQueuesAndComparePeriodically, this),
      getDefaultLitePullConsumerConfig()->topic_metadata_check_interval_millis(), time_unit::milliseconds);
}

void DefaultLitePullConsumerImpl::fetchTopicMessageQueuesAndCompare() {
  std::lock_guard<std::mutex> lock(mutex_);  // synchronized
  for (const auto& it : topic_message_queue_change_listener_map_) {
    const auto& topic = it.first;
    auto* topic_message_queue_change_listener = it.second;
    std::vector<MQMessageQueue> old_message_queues = message_queues_for_topic_[topic];
    std::vector<MQMessageQueue> new_message_queues = fetchMessageQueues(topic);
    bool isChanged = !isSetEqual(new_message_queues, old_message_queues);
    if (isChanged) {
      message_queues_for_topic_[topic] = new_message_queues;
      if (topic_message_queue_change_listener != nullptr) {
        topic_message_queue_change_listener->onChanged(topic, new_message_queues);
      }
    }
  }
}

bool DefaultLitePullConsumerImpl::isSetEqual(std::vector<MQMessageQueue>& new_message_queues,
                                             std::vector<MQMessageQueue>& old_message_queues) {
  if (new_message_queues.size() != old_message_queues.size()) {
    return false;
  }
  std::sort(new_message_queues.begin(), new_message_queues.end());
  std::sort(old_message_queues.begin(), old_message_queues.end());
  return new_message_queues == old_message_queues;
}

void DefaultLitePullConsumerImpl::operateAfterRunning() {
  // If subscribe function invoke before start function, then update topic subscribe info after initialization.
  if (subscription_type_ == SubscriptionType::SUBSCRIBE) {
    updateTopicSubscribeInfoWhenSubscriptionChanged();
  }
  // If assign function invoke before start function, then update pull task after initialization.
  else if (subscription_type_ == SubscriptionType::ASSIGN) {
    auto message_queues = assigned_message_queue_->messageQueues();
    updateAssignPullTask(message_queues);
  }

  for (const auto& it : topic_message_queue_change_listener_map_) {
    const auto& topic = it.first;
    auto messageQueues = fetchMessageQueues(topic);
    message_queues_for_topic_[topic] = std::move(messageQueues);
  }
  // client_instance_->checkClientInBroker();
}

void DefaultLitePullConsumerImpl::updateTopicSubscribeInfoWhenSubscriptionChanged() {
  auto& subTable = rebalance_impl_->getSubscriptionInner();
  for (const auto& it : subTable) {
    const auto& topic = it.first;
    bool ret = client_instance_->updateTopicRouteInfoFromNameServer(topic);
    if (!ret) {
      LOG_WARN_NEW("The topic:[{}] not exist", topic);
    }
  }
}

void DefaultLitePullConsumerImpl::updateAssignPullTask(std::vector<MQMessageQueue>& mq_new_set) {
  std::sort(mq_new_set.begin(), mq_new_set.end());
  std::lock_guard<std::mutex> lock(task_table_mutex_);
  for (auto it = task_table_.begin(); it != task_table_.end();) {
    auto& mq = it->first;
    if (!std::binary_search(mq_new_set.begin(), mq_new_set.end(), mq)) {
      it->second->set_cancelled(true);
      it = task_table_.erase(it);
      continue;
    }
    it++;
  }
  startPullTask(mq_new_set);
}

void DefaultLitePullConsumerImpl::shutdown() {
  switch (service_state_) {
    case CREATE_JUST:
      break;
    case RUNNING:
      persistConsumerOffset();
      client_instance_->unregisterConsumer(client_config_->group_name());
      scheduled_thread_pool_executor_.shutdown();
      scheduled_executor_service_.shutdown();
      client_instance_->shutdown();
      rebalance_impl_->destroy();
      service_state_ = ServiceState::SHUTDOWN_ALREADY;
      LOG_INFO_NEW("the consumer [{}] shutdown OK", client_config_->group_name());
      break;
    default:
      break;
  }
}

void DefaultLitePullConsumerImpl::subscribe(const std::string& topic, const std::string& subExpression) {
  std::lock_guard<std::mutex> lock(mutex_);  // synchronized
  try {
    if (topic.empty()) {
      THROW_MQEXCEPTION(MQClientException, "Topic can not be null or empty.", -1);
    }
    set_subscription_type(SubscriptionType::SUBSCRIBE);
    auto* subscription_data = FilterAPI::buildSubscriptionData(topic, subExpression);
    rebalance_impl_->setSubscriptionData(topic, subscription_data);

    message_queue_listener_.reset(new MessageQueueListenerImpl(shared_from_this()));
    assigned_message_queue_->set_rebalance_impl(rebalance_impl_.get());

    if (service_state_ == ServiceState::RUNNING) {
      client_instance_->sendHeartbeatToAllBrokerWithLock();
      updateTopicSubscribeInfoWhenSubscriptionChanged();
    }
  } catch (std::exception& e) {
    THROW_MQEXCEPTION2(MQClientException, "subscribe exception", -1, std::make_exception_ptr(e));
  }
}

void DefaultLitePullConsumerImpl::subscribe(const std::string& topic, const MessageSelector& selector) {
  // TODO:
}

void DefaultLitePullConsumerImpl::unsubscribe(const std::string& topic) {
  // TODO:
}

std::vector<SubscriptionData> DefaultLitePullConsumerImpl::subscriptions() const {
  std::vector<SubscriptionData> result;
  auto& subTable = rebalance_impl_->getSubscriptionInner();
  for (const auto& it : subTable) {
    result.push_back(*(it.second));
  }
  return result;
}

void DefaultLitePullConsumerImpl::updateTopicSubscribeInfo(const std::string& topic,
                                                           std::vector<MQMessageQueue>& info) {
  rebalance_impl_->setTopicSubscribeInfo(topic, info);
}

void DefaultLitePullConsumerImpl::doRebalance() {
  if (rebalance_impl_ != nullptr) {
    rebalance_impl_->doRebalance(false);
  }
}

void DefaultLitePullConsumerImpl::updateAssignedMessageQueue(const std::string& topic,
                                                             std::vector<MQMessageQueue>& assigned_message_queue) {
  assigned_message_queue_->updateAssignedMessageQueue(topic, assigned_message_queue);
}

void DefaultLitePullConsumerImpl::updatePullTask(const std::string& topic, std::vector<MQMessageQueue>& mq_new_set) {
  std::sort(mq_new_set.begin(), mq_new_set.end());
  std::lock_guard<std::mutex> lock(task_table_mutex_);
  for (auto it = task_table_.begin(); it != task_table_.end();) {
    auto& mq = it->first;
    if (mq.topic() == topic) {
      // remove unnecessary PullTask
      if (!std::binary_search(mq_new_set.begin(), mq_new_set.end(), mq)) {
        it->second->set_cancelled(true);
        it = task_table_.erase(it);
        continue;
      }
    }
    it++;
  }
  startPullTask(mq_new_set);
}

void DefaultLitePullConsumerImpl::startPullTask(std::vector<MQMessageQueue>& mq_set) {
  for (const auto& mq : mq_set) {
    // add new PullTask
    if (task_table_.find(mq) == task_table_.end()) {
      auto pull_task = std::make_shared<PullTaskImpl>(shared_from_this(), mq);
      task_table_.emplace(mq, pull_task);
      scheduled_thread_pool_executor_.submit(std::bind(&PullTaskImpl::run, pull_task));
    }
  }
}

int64_t DefaultLitePullConsumerImpl::nextPullOffset(const MQMessageQueue& message_queue) {
  int64_t offset = -1;
  int64_t seek_offset = assigned_message_queue_->getSeekOffset(message_queue);
  if (seek_offset != -1) {
    offset = seek_offset;
    assigned_message_queue_->updateConsumeOffset(message_queue, offset);
    assigned_message_queue_->setSeekOffset(message_queue, -1);
  } else {
    offset = assigned_message_queue_->getPullOffset(message_queue);
    if (offset == -1) {
      offset = fetchConsumeOffset(message_queue);
    }
  }
  return offset;
}

int64_t DefaultLitePullConsumerImpl::fetchConsumeOffset(const MQMessageQueue& messageQueue) {
  // checkServiceState();
  return rebalance_impl_->computePullFromWhere(messageQueue);
}

PullResult* DefaultLitePullConsumerImpl::pull(const MQMessageQueue& mq,
                                              SubscriptionData* subscription_data,
                                              int64_t offset,
                                              int max_nums) {
  return pull(mq, subscription_data, offset, max_nums,
              getDefaultLitePullConsumerConfig()->consumer_pull_timeout_millis());
}

PullResult* DefaultLitePullConsumerImpl::pull(const MQMessageQueue& mq,
                                              SubscriptionData* subscription_data,
                                              int64_t offset,
                                              int max_nums,
                                              long timeout) {
  return pullSyncImpl(mq, subscription_data, offset, max_nums,
                      getDefaultLitePullConsumerConfig()->long_polling_enable(), timeout);
}

PullResult* DefaultLitePullConsumerImpl::pullSyncImpl(const MQMessageQueue& mq,
                                                      SubscriptionData* subscription_data,
                                                      int64_t offset,
                                                      int max_nums,
                                                      bool block,
                                                      long timeout) {
  if (offset < 0) {
    THROW_MQEXCEPTION(MQClientException, "offset < 0", -1);
  }

  if (max_nums <= 0) {
    THROW_MQEXCEPTION(MQClientException, "maxNums <= 0", -1);
  }

  int sysFlag = PullSysFlag::buildSysFlag(false, block, true, false, true);

  long timeoutMillis = block ? getDefaultLitePullConsumerConfig()->consumer_timeout_millis_when_suspend() : timeout;

  bool isTagType = ExpressionType::isTagType(subscription_data->expression_type());

  std::unique_ptr<PullResult> pull_result(pull_api_wrapper_->pullKernelImpl(
      mq,                                                                    // mq
      subscription_data->sub_string(),                                       // subExpression
      subscription_data->expression_type(),                                  // expressionType
      isTagType ? 0L : subscription_data->sub_version(),                     // subVersion
      offset,                                                                // offset
      max_nums,                                                              // maxNums
      sysFlag,                                                               // sysFlag
      0,                                                                     // commitOffset
      getDefaultLitePullConsumerConfig()->broker_suspend_max_time_millis(),  // brokerSuspendMaxTimeMillis
      timeoutMillis,                                                         // timeoutMillis
      CommunicationMode::SYNC,                                               // communicationMode
      nullptr));                                                             // pullCallback

  return pull_api_wrapper_->processPullResult(mq, std::move(pull_result), subscription_data);
}

void DefaultLitePullConsumerImpl::submitConsumeRequest(ConsumeRequest* consume_request) {
  consume_request_cache_.push_back(consume_request);
}

void DefaultLitePullConsumerImpl::updatePullOffset(const MQMessageQueue& message_queue, int64_t next_pull_offset) {
  if (assigned_message_queue_->getSeekOffset(message_queue) == -1) {
    assigned_message_queue_->updatePullOffset(message_queue, next_pull_offset);
  }
}

std::vector<MQMessageExt> DefaultLitePullConsumerImpl::poll() {
  return poll(getDefaultLitePullConsumerConfig()->poll_timeout_millis());
}

std::vector<MQMessageExt> DefaultLitePullConsumerImpl::poll(long timeout) {
  // checkServiceState();
  if (auto_commit_) {
    maybeAutoCommit();
  }

  int64_t endTime = UtilAll::currentTimeMillis() + timeout;

  auto consume_request = consume_request_cache_.pop_front(timeout, time_unit::milliseconds);
  if (endTime - UtilAll::currentTimeMillis() > 0) {
    while (consume_request != nullptr && consume_request->process_queue()->dropped()) {
      consume_request = consume_request_cache_.pop_front();
      if (endTime - UtilAll::currentTimeMillis() <= 0) {
        break;
      }
    }
  }

  if (consume_request != nullptr && !consume_request->process_queue()->dropped()) {
    auto& messages = consume_request->message_exts();
    long offset = consume_request->process_queue()->removeMessage(messages);
    assigned_message_queue_->updateConsumeOffset(consume_request->message_queue(), offset);
    // If namespace not null , reset Topic without namespace.
    resetTopic(messages);
    return MQMessageExt::from_list(messages);
  }

  return std::vector<MQMessageExt>();
}

void DefaultLitePullConsumerImpl::maybeAutoCommit() {
  auto now = UtilAll::currentTimeMillis();
  if (now >= next_auto_commit_deadline_) {
    commitAll();
    next_auto_commit_deadline_ = now + getDefaultLitePullConsumerConfig()->auto_commit_interval_millis();
  }
}

void DefaultLitePullConsumerImpl::resetTopic(std::vector<MessageExtPtr>& msg_list) {
  if (msg_list.empty()) {
    return;
  }

  // If namespace not null , reset Topic without namespace.
  const auto& name_space = getDefaultLitePullConsumerConfig()->name_space();
  if (!name_space.empty()) {
    for (auto& message_ext : msg_list) {
      message_ext->set_topic(NamespaceUtil::withoutNamespace(message_ext->topic(), name_space));
    }
  }
}

void DefaultLitePullConsumerImpl::commitAll() {
  try {
    std::vector<MQMessageQueue> message_queues = assigned_message_queue_->messageQueues();
    for (const auto& message_queue : message_queues) {
      long consumer_offset = assigned_message_queue_->getConsumerOffset(message_queue);
      if (consumer_offset != -1) {
        auto process_queue = assigned_message_queue_->getProcessQueue(message_queue);
        if (process_queue != nullptr && !process_queue->dropped()) {
          updateConsumeOffset(message_queue, consumer_offset);
        }
      }
    }
    if (getDefaultLitePullConsumerConfig()->message_model() == MessageModel::BROADCASTING) {
      offset_store_->persistAll(message_queues);
    }
  } catch (std::exception& e) {
    LOG_ERROR_NEW("An error occurred when update consume offset Automatically.");
  }
}

void DefaultLitePullConsumerImpl::updateConsumeOffset(const MQMessageQueue& mq, int64_t offset) {
  // checkServiceState();
  offset_store_->updateOffset(mq, offset, false);
}

void DefaultLitePullConsumerImpl::persistConsumerOffset() {
  if (isServiceStateOk()) {
    std::vector<MQMessageQueue> allocated_mqs;
    if (subscription_type_ == SubscriptionType::SUBSCRIBE) {
      allocated_mqs = rebalance_impl_->getAllocatedMQ();
    } else if (subscription_type_ == SubscriptionType::ASSIGN) {
      allocated_mqs = assigned_message_queue_->messageQueues();
    }
    offset_store_->persistAll(allocated_mqs);
  }
}

std::vector<MQMessageQueue> DefaultLitePullConsumerImpl::fetchMessageQueues(const std::string& topic) {
  std::vector<MQMessageQueue> result;
  if (isServiceStateOk()) {
    client_instance_->getMQAdminImpl()->fetchSubscribeMessageQueues(topic, result);
    parseMessageQueues(result);
  }
  return result;
}

void DefaultLitePullConsumerImpl::parseMessageQueues(std::vector<MQMessageQueue>& queueSet) {
  const auto& name_space = client_config_->name_space();
  if (name_space.empty()) {
    return;
  }
  for (auto& messageQueue : queueSet) {
    auto user_topic = NamespaceUtil::withoutNamespace(messageQueue.topic(), name_space);
    messageQueue.set_topic(user_topic);
  }
}

void DefaultLitePullConsumerImpl::assign(const std::vector<MQMessageQueue>& messageQueues) {
  // TODO:
}

void DefaultLitePullConsumerImpl::seek(const MQMessageQueue& messageQueue, int64_t offset) {
  // TODO:
}

void DefaultLitePullConsumerImpl::seekToBegin(const MQMessageQueue& message_queue) {
  auto begin = minOffset(message_queue);
  seek(message_queue, begin);
}

void DefaultLitePullConsumerImpl::seekToEnd(const MQMessageQueue& message_queue) {
  auto end = maxOffset(message_queue);
  seek(message_queue, end);
}

int64_t DefaultLitePullConsumerImpl::offsetForTimestamp(const MQMessageQueue& message_queue, int64_t timestamp) {
  return searchOffset(message_queue, timestamp);
}

void DefaultLitePullConsumerImpl::pause(const std::vector<MQMessageQueue>& message_queues) {
  assigned_message_queue_->pause(message_queues);
}

void DefaultLitePullConsumerImpl::resume(const std::vector<MQMessageQueue>& message_queues) {
  assigned_message_queue_->resume(message_queues);
}

void DefaultLitePullConsumerImpl::commitSync() {
  commitAll();
}

int64_t DefaultLitePullConsumerImpl::committed(const MQMessageQueue& message_queue) {
  // checkServiceState();
  auto offset = offset_store_->readOffset(message_queue, ReadOffsetType::MEMORY_FIRST_THEN_STORE);
  if (offset == -2) {
    THROW_MQEXCEPTION(MQClientException, "Fetch consume offset from broker exception", -1);
  }
  return offset;
}

void DefaultLitePullConsumerImpl::registerTopicMessageQueueChangeListener(
    const std::string& topic,
    TopicMessageQueueChangeListener* topicMessageQueueChangeListener) {
  std::lock_guard<std::mutex> lock(mutex_);  // synchronized
  if (topic.empty() || nullptr == topicMessageQueueChangeListener) {
    THROW_MQEXCEPTION(MQClientException, "Topic or listener is null", -1);
  }
  if (topic_message_queue_change_listener_map_.find(topic) != topic_message_queue_change_listener_map_.end()) {
    LOG_WARN_NEW("Topic {} had been registered, new listener will overwrite the old one", topic);
  }

  topic_message_queue_change_listener_map_[topic] = topicMessageQueueChangeListener;
  if (service_state_ == ServiceState::RUNNING) {
    auto messageQueues = fetchMessageQueues(topic);
    message_queues_for_topic_[topic] = std::move(messageQueues);
  }
}

ConsumerRunningInfo* DefaultLitePullConsumerImpl::consumerRunningInfo() {
  auto* info = new ConsumerRunningInfo();

  info->setProperty(ConsumerRunningInfo::PROP_CONSUMER_START_TIMESTAMP, UtilAll::to_string(start_time_));

  info->setSubscriptionSet(subscriptions());

  auto processQueueTable = rebalance_impl_->getProcessQueueTable();
  for (const auto& it : processQueueTable) {
    const auto& mq = it.first;
    const auto& pq = it.second;

    ProcessQueueInfo pq_info;
    pq_info.setCommitOffset(offset_store_->readOffset(mq, MEMORY_FIRST_THEN_STORE));
    pq->fillProcessQueueInfo(pq_info);
    info->setMqTable(mq, pq_info);
  }

  return info;
}

bool DefaultLitePullConsumerImpl::isAutoCommit() const {
  return auto_commit_;
}

void DefaultLitePullConsumerImpl::setAutoCommit(bool auto_commit) {
  auto_commit_ = auto_commit;
}

const std::string& DefaultLitePullConsumerImpl::groupName() const {
  return client_config_->group_name();
}

MessageModel DefaultLitePullConsumerImpl::messageModel() const {
  return getDefaultLitePullConsumerConfig()->message_model();
};

ConsumeType DefaultLitePullConsumerImpl::consumeType() const {
  return CONSUME_ACTIVELY;
}

ConsumeFromWhere DefaultLitePullConsumerImpl::consumeFromWhere() const {
  return getDefaultLitePullConsumerConfig()->consume_from_where();
}

void DefaultLitePullConsumerImpl::set_subscription_type(SubscriptionType subscription_type) {
  if (subscription_type_ == SubscriptionType::NONE) {
    subscription_type_ = subscription_type;
  } else if (subscription_type_ != subscription_type) {
    THROW_MQEXCEPTION(MQClientException, "Subscribe and assign are mutually exclusive.", -1);
  }
}

}  // namespace rocketmq
