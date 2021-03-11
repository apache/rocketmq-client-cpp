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
#include "DefaultMQPushConsumerImpl.h"

#ifndef WIN32
#include <signal.h>
#endif

#include "CommunicationMode.h"
#include "ConsumeMsgService.h"
#include "protocol/body/ConsumerRunningInfo.h"
#include "FilterAPI.hpp"
#include "Logging.h"
#include "MQAdminImpl.h"
#include "MQClientAPIImpl.h"
#include "MQClientInstance.h"
#include "MQClientManager.h"
#include "MQProtos.h"
#include "NamespaceUtil.h"
#include "LocalFileOffsetStore.h"
#include "PullAPIWrapper.h"
#include "PullMessageService.hpp"
#include "PullSysFlag.h"
#include "RebalancePushImpl.h"
#include "RemoteBrokerOffsetStore.h"
#include "SocketUtil.h"
#include "UtilAll.h"
#include "Validators.h"

static const long BROKER_SUSPEND_MAX_TIME_MILLIS = 1000 * 15;
static const long CONSUMER_TIMEOUT_MILLIS_WHEN_SUSPEND = 1000 * 30;

namespace rocketmq {

class DefaultMQPushConsumerImpl::AsyncPullCallback : public AutoDeletePullCallback {
 public:
  AsyncPullCallback(DefaultMQPushConsumerImplPtr pushConsumer,
                    PullRequestPtr request,
                    SubscriptionData* subscriptionData)
      : default_mq_push_consumer_(pushConsumer), pull_request_(request), subscription_data_(subscriptionData) {}

  ~AsyncPullCallback() = default;

  void onSuccess(std::unique_ptr<PullResult> pull_result) override {
    auto consumer = default_mq_push_consumer_.lock();
    if (nullptr == consumer) {
      LOG_WARN_NEW("AsyncPullCallback::onSuccess: DefaultMQPushConsumerImpl is released.");
      return;
    }

    pull_result.reset(consumer->pull_api_wrapper_->processPullResult(pull_request_->message_queue(),
                                                                     std::move(pull_result), subscription_data_));
    switch (pull_result->pull_status()) {
      case FOUND: {
        int64_t prev_request_offset = pull_request_->next_offset();
        pull_request_->set_next_offset(pull_result->next_begin_offset());

        int64_t first_msg_offset = (std::numeric_limits<int64_t>::max)();
        if (!pull_result->msg_found_list().empty()) {
          first_msg_offset = pull_result->msg_found_list()[0]->queue_offset();

          pull_request_->process_queue()->putMessage(pull_result->msg_found_list());
          consumer->consume_service_->submitConsumeRequest(
              pull_result->msg_found_list(), pull_request_->process_queue(), pull_request_->message_queue(), true);
        }

        consumer->executePullRequestImmediately(pull_request_);

        if (pull_result->next_begin_offset() < prev_request_offset || first_msg_offset < prev_request_offset) {
          LOG_WARN_NEW(
              "[BUG] pull message result maybe data wrong, nextBeginOffset:{} firstMsgOffset:{} prevRequestOffset:{}",
              pull_result->next_begin_offset(), first_msg_offset, prev_request_offset);
        }
      } break;
      case NO_NEW_MSG:
      case NO_MATCHED_MSG:
        pull_request_->set_next_offset(pull_result->next_begin_offset());
        consumer->correctTagsOffset(pull_request_);
        consumer->executePullRequestImmediately(pull_request_);
        break;
      case NO_LATEST_MSG:
        pull_request_->set_next_offset(pull_result->next_begin_offset());
        consumer->correctTagsOffset(pull_request_);
        consumer->executePullRequestLater(
            pull_request_, consumer->getDefaultMQPushConsumerConfig()->pull_time_delay_millis_when_exception());
        break;
      case OFFSET_ILLEGAL: {
        LOG_WARN_NEW("the pull request offset illegal, {} {}", pull_request_->toString(), pull_result->toString());

        pull_request_->set_next_offset(pull_result->next_begin_offset());
        pull_request_->process_queue()->set_dropped(true);

        // update and persist offset, then removeProcessQueue
        auto pull_request = pull_request_;
        consumer->executeTaskLater(
            [consumer, pull_request]() {
              try {
                consumer->getOffsetStore()->updateOffset(pull_request->message_queue(), pull_request->next_offset(),
                                                         false);
                consumer->getOffsetStore()->persist(pull_request->message_queue());
                consumer->getRebalanceImpl()->removeProcessQueue(pull_request->message_queue());

                LOG_WARN_NEW("fix the pull request offset, {}", pull_request->toString());
              } catch (std::exception& e) {
                LOG_ERROR_NEW("executeTaskLater Exception: {}", e.what());
              }
            },
            10000);
      } break;
      default:
        break;
    }
  }

  void onException(MQException& e) noexcept override {
    auto consumer = default_mq_push_consumer_.lock();
    if (nullptr == consumer) {
      LOG_WARN_NEW("AsyncPullCallback::onException: DefaultMQPushConsumerImpl is released.");
      return;
    }

    if (!UtilAll::isRetryTopic(pull_request_->message_queue().topic())) {
      LOG_WARN_NEW("execute the pull request exception: {}", e.what());
    }

    // TODO
    consumer->executePullRequestLater(
        pull_request_, consumer->getDefaultMQPushConsumerConfig()->pull_time_delay_millis_when_exception());
  }

 private:
  std::weak_ptr<DefaultMQPushConsumerImpl> default_mq_push_consumer_;
  PullRequestPtr pull_request_;
  SubscriptionData* subscription_data_;
};

DefaultMQPushConsumerImpl::DefaultMQPushConsumerImpl(DefaultMQPushConsumerConfigPtr config)
    : DefaultMQPushConsumerImpl(config, nullptr) {}

DefaultMQPushConsumerImpl::DefaultMQPushConsumerImpl(DefaultMQPushConsumerConfigPtr config, RPCHookPtr rpcHook)
    : MQClientImpl(config, rpcHook),
      start_time_(UtilAll::currentTimeMillis()),
      pause_(false),
      consume_orderly_(false),
      message_listener_(nullptr),
      consume_service_(nullptr),
      rebalance_impl_(new RebalancePushImpl(this)),
      pull_api_wrapper_(nullptr),
      offset_store_(nullptr) {}

DefaultMQPushConsumerImpl::~DefaultMQPushConsumerImpl() = default;

void DefaultMQPushConsumerImpl::start() {
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

      copySubscription();

      if (messageModel() == MessageModel::CLUSTERING) {
        client_config_->changeInstanceNameToPID();
      }

      // init client_instance_
      MQClientImpl::start();

      // init rebalance_impl_
      rebalance_impl_->set_consumer_group(client_config_->group_name());
      rebalance_impl_->set_message_model(getDefaultMQPushConsumerConfig()->message_model());
      rebalance_impl_->set_allocate_mq_strategy(getDefaultMQPushConsumerConfig()->allocate_mq_strategy());
      rebalance_impl_->set_client_instance(client_instance_.get());

      // init pull_api_wrapper_
      pull_api_wrapper_.reset(new PullAPIWrapper(client_instance_.get(), client_config_->group_name()));
      // TODO: registerFilterMessageHook

      // init offset_store_
      switch (getDefaultMQPushConsumerConfig()->message_model()) {
        case MessageModel::BROADCASTING:
          offset_store_.reset(new LocalFileOffsetStore(client_instance_.get(), client_config_->group_name()));
          break;
        case MessageModel::CLUSTERING:
          offset_store_.reset(new RemoteBrokerOffsetStore(client_instance_.get(), client_config_->group_name()));
          break;
      }
      offset_store_->load();

      // checkConfig() guarantee message_listener_ is not nullptr
      if (message_listener_->getMessageListenerType() == messageListenerOrderly) {
        LOG_INFO_NEW("start orderly consume service: {}", client_config_->group_name());
        consume_orderly_ = true;
        consume_service_.reset(new ConsumeMessageOrderlyService(
            this, getDefaultMQPushConsumerConfig()->consume_thread_nums(), message_listener_));
      } else {
        // for backward compatible, defaultly and concurrently listeners are allocating
        // ConsumeMessageConcurrentlyService
        LOG_INFO_NEW("start concurrently consume service: {}", client_config_->group_name());
        consume_orderly_ = false;
        consume_service_.reset(new ConsumeMessageConcurrentlyService(
            this, getDefaultMQPushConsumerConfig()->consume_thread_nums(), message_listener_));
      }
      consume_service_->start();

      // register consumer
      bool registerOK = client_instance_->registerConsumer(client_config_->group_name(), this);
      if (!registerOK) {
        service_state_ = CREATE_JUST;
        consume_service_->shutdown();
        THROW_MQEXCEPTION(MQClientException, "The cousumer group[" + client_config_->group_name() +
                                                 "] has been created before, specify another name please.",
                          -1);
      }

      client_instance_->start();

      LOG_INFO_NEW("the consumer [{}] start OK", client_config_->group_name());
      service_state_ = RUNNING;
      break;
    }
    case RUNNING:
    case START_FAILED:
    case SHUTDOWN_ALREADY:
      THROW_MQEXCEPTION(MQClientException, "The PushConsumer service state not OK, maybe started once", -1);
      break;
    default:
      break;
  }

  updateTopicSubscribeInfoWhenSubscriptionChanged();
  client_instance_->sendHeartbeatToAllBrokerWithLock();
  client_instance_->rebalanceImmediately();
}

void DefaultMQPushConsumerImpl::checkConfig() {
  std::string groupname = client_config_->group_name();

  // check consumerGroup
  Validators::checkGroup(groupname);

  // consumerGroup
  if (DEFAULT_CONSUMER_GROUP == groupname) {
    THROW_MQEXCEPTION(MQClientException,
                      "consumerGroup can not equal " + DEFAULT_CONSUMER_GROUP + ", please specify another one.", -1);
  }

  if (getDefaultMQPushConsumerConfig()->message_model() != BROADCASTING &&
      getDefaultMQPushConsumerConfig()->message_model() != CLUSTERING) {
    THROW_MQEXCEPTION(MQClientException, "messageModel is valid", -1);
  }

  // allocateMessageQueueStrategy
  if (nullptr == getDefaultMQPushConsumerConfig()->allocate_mq_strategy()) {
    THROW_MQEXCEPTION(MQClientException, "allocateMessageQueueStrategy is null", -1);
  }

  // subscription
  if (subscription_.empty()) {
    THROW_MQEXCEPTION(MQClientException, "subscription is empty", -1);
  }

  // messageListener
  if (message_listener_ == nullptr) {
    THROW_MQEXCEPTION(MQClientException, "messageListener is null", -1);
  }
}

void DefaultMQPushConsumerImpl::copySubscription() {
  for (const auto& it : subscription_) {
    LOG_INFO_NEW("buildSubscriptionData: {}, {}", it.first, it.second);
    SubscriptionData* subscriptionData = FilterAPI::buildSubscriptionData(it.first, it.second);
    rebalance_impl_->setSubscriptionData(it.first, subscriptionData);
  }

  switch (getDefaultMQPushConsumerConfig()->message_model()) {
    case BROADCASTING:
      break;
    case CLUSTERING: {
      // auto subscript retry topic
      std::string retryTopic = UtilAll::getRetryTopic(client_config_->group_name());
      SubscriptionData* subscriptionData = FilterAPI::buildSubscriptionData(retryTopic, SUB_ALL);
      rebalance_impl_->setSubscriptionData(retryTopic, subscriptionData);
      break;
    }
    default:
      break;
  }
}

void DefaultMQPushConsumerImpl::updateTopicSubscribeInfoWhenSubscriptionChanged() {
  auto& subTable = rebalance_impl_->getSubscriptionInner();
  for (const auto& it : subTable) {
    const auto& topic = it.first;
    bool ret = client_instance_->updateTopicRouteInfoFromNameServer(topic);
    if (!ret) {
      LOG_WARN_NEW("The topic:[{}] not exist", topic);
    }
  }
}

void DefaultMQPushConsumerImpl::shutdown() {
  switch (service_state_) {
    case RUNNING: {
      consume_service_->shutdown();
      persistConsumerOffset();
      client_instance_->unregisterConsumer(client_config_->group_name());
      client_instance_->shutdown();
      rebalance_impl_->destroy();
      service_state_ = SHUTDOWN_ALREADY;
      LOG_INFO_NEW("the consumer [{}] shutdown OK", client_config_->group_name());
      break;
    }
    case CREATE_JUST:
    case SHUTDOWN_ALREADY:
      break;
    default:
      break;
  }
}

void DefaultMQPushConsumerImpl::suspend() {
  pause_ = true;
  LOG_INFO_NEW("suspend this consumer, {}", client_config_->group_name());
}

void DefaultMQPushConsumerImpl::resume() {
  pause_ = false;
  doRebalance();
  LOG_INFO_NEW("resume this consumer, {}", client_config_->group_name());
}

MQMessageListener* DefaultMQPushConsumerImpl::getMessageListener() const {
  return message_listener_;
}

void DefaultMQPushConsumerImpl::registerMessageListener(MessageListenerConcurrently* message_listener) {
  if (nullptr != message_listener) {
    message_listener_ = message_listener;
  }
}

void DefaultMQPushConsumerImpl::registerMessageListener(MessageListenerOrderly* message_listener) {
  if (nullptr != message_listener) {
    message_listener_ = message_listener;
  }
}

void DefaultMQPushConsumerImpl::subscribe(const std::string& topic, const std::string& subExpression) {
  // TODO: change substation after start
  subscription_[topic] = subExpression;
}

std::vector<SubscriptionData> DefaultMQPushConsumerImpl::subscriptions() const {
  std::vector<SubscriptionData> result;
  auto& subTable = rebalance_impl_->getSubscriptionInner();
  for (const auto& it : subTable) {
    result.push_back(*(it.second));
  }
  return result;
}

void DefaultMQPushConsumerImpl::updateTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& info) {
  rebalance_impl_->setTopicSubscribeInfo(topic, info);
}

void DefaultMQPushConsumerImpl::doRebalance() {
  if (!pause_) {
    rebalance_impl_->doRebalance(consume_orderly());
  }
}

void DefaultMQPushConsumerImpl::executePullRequestLater(PullRequestPtr pullRequest, long timeDelay) {
  client_instance_->getPullMessageService()->executePullRequestLater(pullRequest, timeDelay);
}

void DefaultMQPushConsumerImpl::executePullRequestImmediately(PullRequestPtr pullRequest) {
  client_instance_->getPullMessageService()->executePullRequestImmediately(pullRequest);
}

void DefaultMQPushConsumerImpl::pullMessage(PullRequestPtr pull_request) {
  if (nullptr == pull_request) {
    LOG_ERROR("PullRequest is NULL, return");
    return;
  }

  auto process_queue = pull_request->process_queue();
  if (process_queue->dropped()) {
    LOG_WARN_NEW("the pull request[{}] is dropped.", pull_request->toString());
    return;
  }

  process_queue->set_last_pull_timestamp(UtilAll::currentTimeMillis());

  int cachedMessageCount = process_queue->getCacheMsgCount();
  if (cachedMessageCount > getDefaultMQPushConsumerConfig()->pull_threshold_for_queue()) {
    // too many message in cache, wait to process
    executePullRequestLater(pull_request, 1000);
    return;
  }

  if (consume_orderly()) {
    if (process_queue->locked()) {
      if (!pull_request->locked_first()) {
        const auto offset = rebalance_impl_->computePullFromWhere(pull_request->message_queue());
        bool brokerBusy = offset < pull_request->next_offset();
        LOG_INFO_NEW(
            "the first time to pull message, so fix offset from broker. pullRequest: {} NewOffset: {} brokerBusy: {}",
            pull_request->toString(), offset, UtilAll::to_string(brokerBusy));
        if (brokerBusy) {
          LOG_INFO_NEW(
              "[NOTIFYME] the first time to pull message, but pull request offset larger than broker consume offset. "
              "pullRequest: {} NewOffset: {}",
              pull_request->toString(), offset);
        }

        pull_request->set_locked_first(true);
        pull_request->set_next_offset(offset);
      }
    } else {
      executePullRequestLater(pull_request, getDefaultMQPushConsumerConfig()->pull_time_delay_millis_when_exception());
      LOG_INFO_NEW("pull message later because not locked in broker, {}", pull_request->toString());
      return;
    }
  }

  const auto& message_queue = pull_request->message_queue();
  SubscriptionData* subscription_data = rebalance_impl_->getSubscriptionData(message_queue.topic());
  if (nullptr == subscription_data) {
    executePullRequestLater(pull_request, getDefaultMQPushConsumerConfig()->pull_time_delay_millis_when_exception());
    LOG_WARN_NEW("find the consumer's subscription failed, {}", pull_request->toString());
    return;
  }

  bool commitOffsetEnable = false;
  int64_t commitOffsetValue = 0;
  if (CLUSTERING == getDefaultMQPushConsumerConfig()->message_model()) {
    commitOffsetValue = offset_store_->readOffset(message_queue, READ_FROM_MEMORY);
    if (commitOffsetValue > 0) {
      commitOffsetEnable = true;
    }
  }

  const auto& subExpression = subscription_data->sub_string();

  int sysFlag = PullSysFlag::buildSysFlag(commitOffsetEnable,      // commitOffset
                                          true,                    // suspend
                                          !subExpression.empty(),  // subscription
                                          false);                  // class filter

  try {
    std::unique_ptr<AsyncPullCallback> callback(
        new AsyncPullCallback(shared_from_this(), pull_request, subscription_data));

    pull_api_wrapper_->pullKernelImpl(message_queue,                                        // mq
                                      subExpression,                                        // subExpression
                                      subscription_data->expression_type(),                 // expressionType
                                      subscription_data->sub_version(),                     // subVersion
                                      pull_request->next_offset(),                          // offset
                                      getDefaultMQPushConsumerConfig()->pull_batch_size(),  // maxNums
                                      sysFlag,                                              // sysFlag
                                      commitOffsetValue,                                    // commitOffset
                                      BROKER_SUSPEND_MAX_TIME_MILLIS,        // brokerSuspendMaxTimeMillis
                                      CONSUMER_TIMEOUT_MILLIS_WHEN_SUSPEND,  // timeoutMillis
                                      CommunicationMode::ASYNC,              // communicationMode
                                      callback.get());                       // pullCallback

    (void)callback.release();
  } catch (MQException& e) {
    LOG_ERROR_NEW("pullKernelImpl exception: {}", e.what());
    executePullRequestLater(pull_request, getDefaultMQPushConsumerConfig()->pull_time_delay_millis_when_exception());
  }
}

void DefaultMQPushConsumerImpl::correctTagsOffset(PullRequestPtr pullRequest) {
  if (0L == pullRequest->process_queue()->getCacheMsgCount()) {
    offset_store_->updateOffset(pullRequest->message_queue(), pullRequest->next_offset(), true);
  }
}

void DefaultMQPushConsumerImpl::executeTaskLater(const handler_type& task, long timeDelay) {
  client_instance_->getPullMessageService()->executeTaskLater(task, timeDelay);
}

void DefaultMQPushConsumerImpl::resetRetryAndNamespace(const std::vector<MessageExtPtr>& msgs) {
  std::string retry_topic = UtilAll::getRetryTopic(groupName());
  for (auto& msg : msgs) {
    std::string group_topic = msg->getProperty(MQMessageConst::PROPERTY_RETRY_TOPIC);
    if (!group_topic.empty() && retry_topic == msg->topic()) {
      msg->set_topic(group_topic);
    }
  }

  const auto& name_space = client_config_->name_space();
  if (!name_space.empty()) {
    for (auto& msg : msgs) {
      msg->set_topic(NamespaceUtil::withoutNamespace(msg->topic(), name_space));
    }
  }
}

bool DefaultMQPushConsumerImpl::sendMessageBack(MessageExtPtr msg, int delay_level) {
  return sendMessageBack(msg, delay_level, null);
}

bool DefaultMQPushConsumerImpl::sendMessageBack(MessageExtPtr msg, int delay_level, const std::string& brokerName) {
  try {
    msg->set_topic(NamespaceUtil::wrapNamespace(client_config_->name_space(), msg->topic()));

    std::string brokerAddr =
        brokerName.empty() ? msg->store_host_string() : client_instance_->findBrokerAddressInPublish(brokerName);

    client_instance_->getMQClientAPIImpl()->consumerSendMessageBack(
        brokerAddr, msg, getDefaultMQPushConsumerConfig()->group_name(), delay_level, 5000,
        getDefaultMQPushConsumerConfig()->max_reconsume_times());
    return true;
  } catch (const std::exception& e) {
    LOG_ERROR_NEW("sendMessageBack exception, group: {}, msg: {}. {}", getDefaultMQPushConsumerConfig()->group_name(),
                  msg->toString(), e.what());
  }
  return false;
}

void DefaultMQPushConsumerImpl::persistConsumerOffset() {
  if (isServiceStateOk()) {
    std::vector<MQMessageQueue> mqs = rebalance_impl_->getAllocatedMQ();
    offset_store_->persistAll(mqs);
  }
}

void DefaultMQPushConsumerImpl::updateConsumeOffset(const MQMessageQueue& mq, int64_t offset) {
  if (offset >= 0) {
    offset_store_->updateOffset(mq, offset, false);
  } else {
    LOG_ERROR_NEW("updateConsumeOffset of mq:{} error", mq.toString());
  }
}

ConsumerRunningInfo* DefaultMQPushConsumerImpl::consumerRunningInfo() {
  auto* info = new ConsumerRunningInfo();

  info->setProperty(ConsumerRunningInfo::PROP_CONSUME_ORDERLY, UtilAll::to_string(consume_orderly_));
  info->setProperty(ConsumerRunningInfo::PROP_THREADPOOL_CORE_SIZE,
                    UtilAll::to_string(getDefaultMQPushConsumerConfig()->consume_thread_nums()));
  info->setProperty(ConsumerRunningInfo::PROP_CONSUMER_START_TIMESTAMP, UtilAll::to_string(start_time_));

  auto sub_set = subscriptions();
  info->setSubscriptionSet(sub_set);

  auto processQueueTable = rebalance_impl_->getProcessQueueTable();
  for (const auto& it : processQueueTable) {
    const auto& mq = it.first;
    const auto& pq = it.second;

    ProcessQueueInfo pq_info;
    pq_info.setCommitOffset(offset_store_->readOffset(mq, MEMORY_FIRST_THEN_STORE));
    pq->fillProcessQueueInfo(pq_info);
    info->setMqTable(mq, pq_info);
  }

  // TODO: ConsumeStatus

  return info;
}

const std::string& DefaultMQPushConsumerImpl::groupName() const {
  return client_config_->group_name();
}

MessageModel DefaultMQPushConsumerImpl::messageModel() const {
  return getDefaultMQPushConsumerConfig()->message_model();
};

ConsumeType DefaultMQPushConsumerImpl::consumeType() const {
  return CONSUME_PASSIVELY;
}

ConsumeFromWhere DefaultMQPushConsumerImpl::consumeFromWhere() const {
  return getDefaultMQPushConsumerConfig()->consume_from_where();
}

}  // namespace rocketmq
