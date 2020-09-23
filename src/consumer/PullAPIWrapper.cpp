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
#include "PullAPIWrapper.h"

#include "ByteBuffer.hpp"
#include "MQClientAPIImpl.h"
#include "MQClientInstance.h"
#include "MessageDecoder.h"
#include "MessageAccessor.hpp"
#include "PullResultExt.hpp"
#include "PullSysFlag.h"

namespace rocketmq {

PullAPIWrapper::PullAPIWrapper(MQClientInstance* instance, const std::string& consumerGroup) {
  client_instance_ = instance;
  consumer_group_ = consumerGroup;
}

PullAPIWrapper::~PullAPIWrapper() {
  client_instance_ = nullptr;
  pull_from_which_node_table_.clear();
}

void PullAPIWrapper::updatePullFromWhichNode(const MQMessageQueue& mq, int brokerId) {
  std::lock_guard<std::mutex> lock(lock_);
  pull_from_which_node_table_[mq] = brokerId;
}

int PullAPIWrapper::recalculatePullFromWhichNode(const MQMessageQueue& mq) {
  std::lock_guard<std::mutex> lock(lock_);
  const auto& it = pull_from_which_node_table_.find(mq);
  if (it != pull_from_which_node_table_.end()) {
    return it->second;
  }
  return MASTER_ID;
}

PullResult* PullAPIWrapper::processPullResult(const MQMessageQueue& mq,
                                              std::unique_ptr<PullResult> pull_result,
                                              SubscriptionData* subscription_data) {
  auto* pull_result_ext = dynamic_cast<PullResultExt*>(pull_result.get());
  if (pull_result_ext == nullptr) {
    return pull_result.release();
  }

  // update node
  updatePullFromWhichNode(mq, pull_result_ext->suggert_which_boker_id());

  std::vector<MessageExtPtr> msg_list_filter_again;
  if (FOUND == pull_result_ext->pull_status()) {
    // decode all msg list
    std::unique_ptr<ByteBuffer> byteBuffer(ByteBuffer::wrap(pull_result_ext->message_binary()));
    auto msgList = MessageDecoder::decodes(*byteBuffer);

    // filter msg list again
    if (subscription_data != nullptr && !subscription_data->tags_set().empty()) {
      msg_list_filter_again.reserve(msgList.size());
      for (const auto& msg : msgList) {
        const auto& msgTag = msg->tags();
        if (subscription_data->containsTag(msgTag)) {
          msg_list_filter_again.push_back(msg);
        }
      }
    } else {
      msg_list_filter_again.swap(msgList);
    }

    if (!msg_list_filter_again.empty()) {
      std::string min_offset = UtilAll::to_string(pull_result_ext->min_offset());
      std::string max_offset = UtilAll::to_string(pull_result_ext->max_offset());
      for (auto& msg : msg_list_filter_again) {
        const auto& tranMsg = msg->getProperty(MQMessageConst::PROPERTY_TRANSACTION_PREPARED);
        if (UtilAll::stob(tranMsg)) {
          msg->set_transaction_id(msg->getProperty(MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX));
        }
        MessageAccessor::putProperty(*msg, MQMessageConst::PROPERTY_MIN_OFFSET, min_offset);
        MessageAccessor::putProperty(*msg, MQMessageConst::PROPERTY_MAX_OFFSET, max_offset);
      }
    }
  }

  return new PullResult(pull_result_ext->pull_status(), pull_result_ext->next_begin_offset(),
                        pull_result_ext->min_offset(), pull_result_ext->max_offset(), std::move(msg_list_filter_again));
}

PullResult* PullAPIWrapper::pullKernelImpl(const MQMessageQueue& mq,
                                           const std::string& subExpression,
                                           const std::string& expressionType,
                                           int64_t subVersion,
                                           int64_t offset,
                                           int maxNums,
                                           int sysFlag,
                                           int64_t commitOffset,
                                           int brokerSuspendMaxTimeMillis,
                                           int timeoutMillis,
                                           CommunicationMode communicationMode,
                                           PullCallback* pullCallback) {
  std::unique_ptr<FindBrokerResult> findBrokerResult(
      client_instance_->findBrokerAddressInSubscribe(mq.broker_name(), recalculatePullFromWhichNode(mq), false));
  if (findBrokerResult == nullptr) {
    client_instance_->updateTopicRouteInfoFromNameServer(mq.topic());
    findBrokerResult.reset(
        client_instance_->findBrokerAddressInSubscribe(mq.broker_name(), recalculatePullFromWhichNode(mq), false));
  }

  if (findBrokerResult != nullptr) {
    int sysFlagInner = sysFlag;

    if (findBrokerResult->slave()) {
      sysFlagInner = PullSysFlag::clearCommitOffsetFlag(sysFlagInner);
    }

    PullMessageRequestHeader* pRequestHeader = new PullMessageRequestHeader();
    pRequestHeader->consumerGroup = consumer_group_;
    pRequestHeader->topic = mq.topic();
    pRequestHeader->queueId = mq.queue_id();
    pRequestHeader->queueOffset = offset;
    pRequestHeader->maxMsgNums = maxNums;
    pRequestHeader->sysFlag = sysFlagInner;
    pRequestHeader->commitOffset = commitOffset;
    pRequestHeader->suspendTimeoutMillis = brokerSuspendMaxTimeMillis;
    pRequestHeader->subscription = subExpression;
    pRequestHeader->subVersion = subVersion;

    return client_instance_->getMQClientAPIImpl()->pullMessage(findBrokerResult->broker_addr(), pRequestHeader,
                                                               timeoutMillis, communicationMode, pullCallback);
  }

  THROW_MQEXCEPTION(MQClientException, "The broker [" + mq.broker_name() + "] not exist", -1);
}

}  // namespace rocketmq
