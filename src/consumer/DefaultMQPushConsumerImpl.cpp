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
#include "ConsumerRunningInfo.h"
#include "FilterAPI.hpp"
#include "Logging.h"
#include "MQAdminImpl.h"
#include "MQClientAPIImpl.h"
#include "MQClientInstance.h"
#include "MQClientManager.h"
#include "MQProtos.h"
#include "LocalFileOffsetStore.h"
#include "PullAPIWrapper.h"
#include "PullMessageService.hpp"
#include "PullSysFlag.h"
#include "RebalancePushImpl.h"
#include "RemoteBrokerOffsetStore.h"
#include "SocketUtil.h"
#include "UtilAll.h"
#include "Validators.h"

namespace rocketmq {

class AsyncPullCallback : public AutoDeletePullCallback {
 public:
  AsyncPullCallback(DefaultMQPushConsumerImplPtr pushConsumer,
                    PullRequestPtr request,
                    SubscriptionData* subscriptionData)
      : default_mq_push_consumer_(pushConsumer), pull_request_(request), subscription_data_(subscriptionData) {}

  ~AsyncPullCallback() = default;

  void onSuccess(PullResult& pullResult) override {
    auto defaultMQPushConsumer = default_mq_push_consumer_.lock();
    if (nullptr == defaultMQPushConsumer) {
      LOG_WARN_NEW("AsyncPullCallback::onSuccess: DefaultMQPushConsumerImpl is released.");
      return;
    }

    PullResult result = defaultMQPushConsumer->getPullAPIWrapper()->processPullResult(pull_request_->message_queue(),
                                                                                      pullResult, subscription_data_);
    switch (result.pull_status()) {
      case FOUND: {
        int64_t prevRequestOffset = pull_request_->next_offset();
        pull_request_->set_next_offset(result.next_begin_offset());

        int64_t firstMsgOffset = (std::numeric_limits<int64_t>::max)();
        if (result.msg_found_list().empty()) {
          defaultMQPushConsumer->executePullRequestImmediately(pull_request_);
        } else {
          firstMsgOffset = result.msg_found_list()[0]->getQueueOffset();

          pull_request_->process_queue()->putMessage(result.msg_found_list());
          defaultMQPushConsumer->getConsumerMsgService()->submitConsumeRequest(
              result.msg_found_list(), pull_request_->process_queue(), pull_request_->message_queue(), true);

          defaultMQPushConsumer->executePullRequestImmediately(pull_request_);
        }

        if (result.next_begin_offset() < prevRequestOffset || firstMsgOffset < prevRequestOffset) {
          LOG_WARN_NEW(
              "[BUG] pull message result maybe data wrong, nextBeginOffset:{} firstMsgOffset:{} prevRequestOffset:{}",
              result.next_begin_offset(), firstMsgOffset, prevRequestOffset);
        }

      } break;
      case NO_NEW_MSG:
      case NO_MATCHED_MSG:
        pull_request_->set_next_offset(result.next_begin_offset());
        defaultMQPushConsumer->correctTagsOffset(pull_request_);
        defaultMQPushConsumer->executePullRequestImmediately(pull_request_);
        break;
      case NO_LATEST_MSG:
        pull_request_->set_next_offset(result.next_begin_offset());
        defaultMQPushConsumer->correctTagsOffset(pull_request_);
        defaultMQPushConsumer->executePullRequestLater(
            pull_request_,
            defaultMQPushConsumer->getDefaultMQPushConsumerConfig()->getPullTimeDelayMillsWhenException());
        break;
      case OFFSET_ILLEGAL: {
        LOG_WARN_NEW("the pull request offset illegal, {} {}", pull_request_->toString(), result.toString());

        pull_request_->set_next_offset(result.next_begin_offset());
        pull_request_->process_queue()->set_dropped(true);

        // update and persist offset, then removeProcessQueue
        auto pullRequest = pull_request_;
        defaultMQPushConsumer->executeTaskLater(
            [defaultMQPushConsumer, pullRequest]() {
              try {
                defaultMQPushConsumer->getOffsetStore()->updateOffset(pullRequest->message_queue(),
                                                                      pullRequest->next_offset(), false);
                defaultMQPushConsumer->getOffsetStore()->persist(pullRequest->message_queue());
                defaultMQPushConsumer->getRebalanceImpl()->removeProcessQueue(pullRequest->message_queue());

                LOG_WARN_NEW("fix the pull request offset, {}", pullRequest->toString());
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
    auto defaultMQPushConsumer = default_mq_push_consumer_.lock();
    if (nullptr == defaultMQPushConsumer) {
      LOG_WARN_NEW("AsyncPullCallback::onException: DefaultMQPushConsumerImpl is released.");
      return;
    }

    if (!UtilAll::isRetryTopic(pull_request_->message_queue().topic())) {
      LOG_WARN_NEW("execute the pull request exception: {}", e.what());
    }

    // TODO
    defaultMQPushConsumer->executePullRequestLater(
        pull_request_, defaultMQPushConsumer->getDefaultMQPushConsumerConfig()->getPullTimeDelayMillsWhenException());
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
      rebalance_impl_(new RebalancePushImpl(this)),
      pull_api_wrapper_(nullptr),
      offset_store_(nullptr),
      consume_service_(nullptr),
      message_listener_(nullptr) {}

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
      LOG_INFO_NEW("the consumer [{}] start beginning.", client_config_->getGroupName());

      service_state_ = START_FAILED;

      checkConfig();

      copySubscription();

      if (messageModel() == MessageModel::CLUSTERING) {
        client_config_->changeInstanceNameToPID();
      }

      // init client_instance_
      MQClientImpl::start();

      // init rebalance_impl_
      rebalance_impl_->set_consumer_group(client_config_->getGroupName());
      rebalance_impl_->set_message_model(
          dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getMessageModel());
      rebalance_impl_->set_allocate_mq_strategy(
          dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getAllocateMQStrategy());
      rebalance_impl_->set_client_instance(client_instance_.get());

      // init pull_api_wrapper_
      pull_api_wrapper_.reset(new PullAPIWrapper(client_instance_.get(), client_config_->getGroupName()));
      // TODO: registerFilterMessageHook

      // init offset_store_
      switch (dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getMessageModel()) {
        case MessageModel::BROADCASTING:
          offset_store_.reset(new LocalFileOffsetStore(client_instance_.get(), client_config_->getGroupName()));
          break;
        case MessageModel::CLUSTERING:
          offset_store_.reset(new RemoteBrokerOffsetStore(client_instance_.get(), client_config_->getGroupName()));
          break;
      }
      offset_store_->load();

      // checkConfig() guarantee message_listener_ is not nullptr
      if (message_listener_->getMessageListenerType() == messageListenerOrderly) {
        LOG_INFO_NEW("start orderly consume service: {}", client_config_->getGroupName());
        consume_orderly_ = true;
        consume_service_.reset(new ConsumeMessageOrderlyService(
            this, dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getConsumeThreadNum(),
            message_listener_));
      } else {
        // for backward compatible, defaultly and concurrently listeners are allocating
        // ConsumeMessageConcurrentlyService
        LOG_INFO_NEW("start concurrently consume service: {}", client_config_->getGroupName());
        consume_orderly_ = false;
        consume_service_.reset(new ConsumeMessageConcurrentlyService(
            this, dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getConsumeThreadNum(),
            message_listener_));
      }
      consume_service_->start();

      // register consumer
      bool registerOK = client_instance_->registerConsumer(client_config_->getGroupName(), this);
      if (!registerOK) {
        service_state_ = CREATE_JUST;
        consume_service_->shutdown();
        THROW_MQEXCEPTION(MQClientException, "The cousumer group[" + client_config_->getGroupName() +
                                                 "] has been created before, specify another name please.",
                          -1);
      }

      client_instance_->start();

      LOG_INFO_NEW("the consumer [{}] start OK", client_config_->getGroupName());
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
  std::string groupname = client_config_->getGroupName();

  // check consumerGroup
  Validators::checkGroup(groupname);

  // consumerGroup
  if (DEFAULT_CONSUMER_GROUP == groupname) {
    THROW_MQEXCEPTION(MQClientException,
                      "consumerGroup can not equal " + DEFAULT_CONSUMER_GROUP + ", please specify another one.", -1);
  }

  if (dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getMessageModel() != BROADCASTING &&
      dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getMessageModel() != CLUSTERING) {
    THROW_MQEXCEPTION(MQClientException, "messageModel is valid", -1);
  }

  // allocateMessageQueueStrategy
  if (nullptr == dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getAllocateMQStrategy()) {
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

  switch (dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getMessageModel()) {
    case BROADCASTING:
      break;
    case CLUSTERING: {
      // auto subscript retry topic
      std::string retryTopic = UtilAll::getRetryTopic(client_config_->getGroupName());
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
      client_instance_->unregisterConsumer(client_config_->getGroupName());
      client_instance_->shutdown();
      LOG_INFO_NEW("the consumer [{}] shutdown OK", client_config_->getGroupName());
      rebalance_impl_->destroy();
      service_state_ = SHUTDOWN_ALREADY;
      break;
    }
    case CREATE_JUST:
    case SHUTDOWN_ALREADY:
      break;
    default:
      break;
  }
}

bool DefaultMQPushConsumerImpl::sendMessageBack(MessageExtPtr msg, int delayLevel) {
  return sendMessageBack(msg, delayLevel, null);
}

bool DefaultMQPushConsumerImpl::sendMessageBack(MessageExtPtr msg, int delayLevel, const std::string& brokerName) {
  try {
    std::string brokerAddr = brokerName.empty() ? socketAddress2String(msg->getStoreHost())
                                                : client_instance_->findBrokerAddressInPublish(brokerName);

    client_instance_->getMQClientAPIImpl()->consumerSendMessageBack(
        brokerAddr, msg, dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getGroupName(), delayLevel,
        5000, getMaxReconsumeTimes());
    return true;
  } catch (const std::exception& e) {
    LOG_ERROR_NEW("sendMessageBack exception, group: {}, msg: {}. {}",
                  dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getGroupName(), msg->toString(),
                  e.what());
  }
  return false;
}

void DefaultMQPushConsumerImpl::fetchSubscribeMessageQueues(const std::string& topic,
                                                            std::vector<MQMessageQueue>& mqs) {
  mqs.clear();
  try {
    client_instance_->getMQAdminImpl()->fetchSubscribeMessageQueues(topic, mqs);
  } catch (MQException& e) {
    LOG_ERROR_NEW("{}", e.what());
  }
}

void DefaultMQPushConsumerImpl::registerMessageListener(MQMessageListener* messageListener) {
  if (nullptr != messageListener) {
    message_listener_ = messageListener;
  }
}

void DefaultMQPushConsumerImpl::registerMessageListener(MessageListenerConcurrently* messageListener) {
  registerMessageListener(static_cast<MQMessageListener*>(messageListener));
}

void DefaultMQPushConsumerImpl::registerMessageListener(MessageListenerOrderly* messageListener) {
  registerMessageListener(static_cast<MQMessageListener*>(messageListener));
}

MQMessageListener* DefaultMQPushConsumerImpl::getMessageListener() const {
  return message_listener_;
}

void DefaultMQPushConsumerImpl::subscribe(const std::string& topic, const std::string& subExpression) {
  // TODO: change substation after start
  subscription_[topic] = subExpression;
}

void DefaultMQPushConsumerImpl::suspend() {
  pause_ = true;
  LOG_INFO_NEW("suspend this consumer, {}", client_config_->getGroupName());
}

void DefaultMQPushConsumerImpl::resume() {
  pause_ = false;
  doRebalance();
  LOG_INFO_NEW("resume this consumer, {}", client_config_->getGroupName());
}

void DefaultMQPushConsumerImpl::doRebalance() {
  if (!pause_) {
    rebalance_impl_->doRebalance(isConsumeOrderly());
  }
}

void DefaultMQPushConsumerImpl::persistConsumerOffset() {
  if (isServiceStateOk()) {
    std::vector<MQMessageQueue> mqs = rebalance_impl_->getAllocatedMQ();
    if (dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getMessageModel() == BROADCASTING) {
      offset_store_->persistAll(mqs);
    } else {
      for (const auto& mq : mqs) {
        offset_store_->persist(mq);
      }
    }
  }
}

void DefaultMQPushConsumerImpl::updateTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& info) {
  rebalance_impl_->setTopicSubscribeInfo(topic, info);
}

void DefaultMQPushConsumerImpl::updateConsumeOffset(const MQMessageQueue& mq, int64_t offset) {
  if (offset >= 0) {
    offset_store_->updateOffset(mq, offset, false);
  } else {
    LOG_ERROR_NEW("updateConsumeOffset of mq:{} error", mq.toString());
  }
}

void DefaultMQPushConsumerImpl::correctTagsOffset(PullRequestPtr pullRequest) {
  if (0L == pullRequest->process_queue()->getCacheMsgCount()) {
    offset_store_->updateOffset(pullRequest->message_queue(), pullRequest->next_offset(), true);
  }
}

void DefaultMQPushConsumerImpl::executePullRequestLater(PullRequestPtr pullRequest, long timeDelay) {
  client_instance_->getPullMessageService()->executePullRequestLater(pullRequest, timeDelay);
}

void DefaultMQPushConsumerImpl::executePullRequestImmediately(PullRequestPtr pullRequest) {
  client_instance_->getPullMessageService()->executePullRequestImmediately(pullRequest);
}

void DefaultMQPushConsumerImpl::executeTaskLater(const handler_type& task, long timeDelay) {
  client_instance_->getPullMessageService()->executeTaskLater(task, timeDelay);
}

void DefaultMQPushConsumerImpl::pullMessage(PullRequestPtr pullRequest) {
  if (nullptr == pullRequest) {
    LOG_ERROR("PullRequest is NULL, return");
    return;
  }

  auto processQueue = pullRequest->process_queue();
  if (processQueue->dropped()) {
    LOG_WARN_NEW("the pull request[{}] is dropped.", pullRequest->toString());
    return;
  }

  processQueue->set_last_pull_timestamp(UtilAll::currentTimeMillis());

  int cachedMessageCount = processQueue->getCacheMsgCount();
  if (cachedMessageCount >
      dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getMaxCacheMsgSizePerQueue()) {
    // too many message in cache, wait to process
    executePullRequestLater(pullRequest, 1000);
    return;
  }

  if (isConsumeOrderly()) {
    if (processQueue->locked()) {
      if (!pullRequest->locked_first()) {
        const auto offset = rebalance_impl_->computePullFromWhere(pullRequest->message_queue());
        bool brokerBusy = offset < pullRequest->next_offset();
        LOG_INFO_NEW(
            "the first time to pull message, so fix offset from broker. pullRequest: {} NewOffset: {} brokerBusy: {}",
            pullRequest->toString(), offset, UtilAll::to_string(brokerBusy));
        if (brokerBusy) {
          LOG_INFO_NEW(
              "[NOTIFYME] the first time to pull message, but pull request offset larger than broker consume offset. "
              "pullRequest: {} NewOffset: {}",
              pullRequest->toString(), offset);
        }

        pullRequest->set_locked_first(true);
        pullRequest->set_next_offset(offset);
      }
    } else {
      executePullRequestLater(
          pullRequest,
          dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getPullTimeDelayMillsWhenException());
      LOG_INFO_NEW("pull message later because not locked in broker, {}", pullRequest->toString());
      return;
    }
  }

  const auto& messageQueue = pullRequest->message_queue();
  SubscriptionData* subscriptionData = rebalance_impl_->getSubscriptionData(messageQueue.topic());
  if (nullptr == subscriptionData) {
    executePullRequestLater(
        pullRequest,
        dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getPullTimeDelayMillsWhenException());
    LOG_WARN_NEW("find the consumer's subscription failed, {}", pullRequest->toString());
    return;
  }

  bool commitOffsetEnable = false;
  int64_t commitOffsetValue = 0;
  if (CLUSTERING == dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getMessageModel()) {
    commitOffsetValue = offset_store_->readOffset(messageQueue, READ_FROM_MEMORY);
    if (commitOffsetValue > 0) {
      commitOffsetEnable = true;
    }
  }

  const auto& subExpression = subscriptionData->sub_string();

  int sysFlag = PullSysFlag::buildSysFlag(commitOffsetEnable,      // commitOffset
                                          true,                    // suspend
                                          !subExpression.empty(),  // subscription
                                          false);                  // class filter

  try {
    auto* callback = new AsyncPullCallback(shared_from_this(), pullRequest, subscriptionData);
    pull_api_wrapper_->pullKernelImpl(
        messageQueue,                                                                             // 1
        subExpression,                                                                            // 2
        subscriptionData->sub_version(),                                                          // 3
        pullRequest->next_offset(),                                                               // 4
        32,                                                                                       // 5
        sysFlag,                                                                                  // 6
        commitOffsetValue,                                                                        // 7
        1000 * 15,                                                                                // 8
        dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getAsyncPullTimeout(),  // 9
        CommunicationMode::ASYNC,                                                                 // 10
        callback);                                                                                // 11
  } catch (MQException& e) {
    LOG_ERROR_NEW("pullKernelImpl exception: {}", e.what());
    executePullRequestLater(
        pullRequest,
        dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getPullTimeDelayMillsWhenException());
  }
}

void DefaultMQPushConsumerImpl::resetRetryTopic(const std::vector<MessageExtPtr>& msgs,
                                                const std::string& consumerGroup) {
  std::string groupTopic = UtilAll::getRetryTopic(consumerGroup);
  for (auto& msg : msgs) {
    std::string retryTopic = msg->getProperty(MQMessageConst::PROPERTY_RETRY_TOPIC);
    if (!retryTopic.empty() && groupTopic == msg->getTopic()) {
      msg->setTopic(retryTopic);
    }
  }
}

int DefaultMQPushConsumerImpl::getMaxReconsumeTimes() {
  // default reconsume times: 16
  if (dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getMaxReconsumeTimes() == -1) {
    return 16;
  } else {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getMaxReconsumeTimes();
  }
}

std::string DefaultMQPushConsumerImpl::groupName() const {
  return client_config_->getGroupName();
}

MessageModel DefaultMQPushConsumerImpl::messageModel() const {
  return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getMessageModel();
};

ConsumeType DefaultMQPushConsumerImpl::consumeType() const {
  return CONSUME_PASSIVELY;
}

ConsumeFromWhere DefaultMQPushConsumerImpl::consumeFromWhere() const {
  return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getConsumeFromWhere();
}

std::vector<SubscriptionData> DefaultMQPushConsumerImpl::subscriptions() const {
  std::vector<SubscriptionData> result;
  auto& subTable = rebalance_impl_->getSubscriptionInner();
  for (const auto& it : subTable) {
    result.push_back(*(it.second));
  }
  return result;
}

ConsumerRunningInfo* DefaultMQPushConsumerImpl::consumerRunningInfo() {
  auto* info = new ConsumerRunningInfo();

  info->setProperty(ConsumerRunningInfo::PROP_CONSUME_ORDERLY, UtilAll::to_string(consume_orderly_));
  info->setProperty(
      ConsumerRunningInfo::PROP_THREADPOOL_CORE_SIZE,
      UtilAll::to_string(dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getConsumeThreadNum()));
  info->setProperty(ConsumerRunningInfo::PROP_CONSUMER_START_TIMESTAMP, UtilAll::to_string(start_time_));

  auto subSet = subscriptions();
  info->setSubscriptionSet(subSet);

  auto processQueueTable = rebalance_impl_->getProcessQueueTable();

  for (const auto& it : processQueueTable) {
    const auto& mq = it.first;
    const auto& pq = it.second;

    ProcessQueueInfo pqinfo;
    pqinfo.setCommitOffset(offset_store_->readOffset(mq, MEMORY_FIRST_THEN_STORE));
    pq->fillProcessQueueInfo(pqinfo);
    info->setMqTable(mq, pqinfo);
  }

  return info;
}

}  // namespace rocketmq
