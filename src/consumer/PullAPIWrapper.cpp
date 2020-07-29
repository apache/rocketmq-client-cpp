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

PullResult PullAPIWrapper::processPullResult(const MQMessageQueue& mq,
                                             PullResult& pullResult,
                                             SubscriptionData* subscriptionData) {
  assert(std::type_index(typeid(pullResult)) == std::type_index(typeid(PullResultExt)));
  auto& pullResultExt = dynamic_cast<PullResultExt&>(pullResult);

  // update node
  updatePullFromWhichNode(mq, pullResultExt.suggert_which_boker_id());

  std::vector<MessageExtPtr> msgListFilterAgain;
  if (FOUND == pullResultExt.pull_status()) {
    // decode all msg list
    std::unique_ptr<ByteBuffer> byteBuffer(ByteBuffer::wrap(pullResultExt.message_binary()));
    auto msgList = MessageDecoder::decodes(*byteBuffer);

    // filter msg list again
    if (subscriptionData != nullptr && !subscriptionData->tags_set().empty()) {
      msgListFilterAgain.reserve(msgList.size());
      for (const auto& msg : msgList) {
        const auto& msgTag = msg->tags();
        if (subscriptionData->contain_tag(msgTag)) {
          msgListFilterAgain.push_back(msg);
        }
      }
    } else {
      msgListFilterAgain.swap(msgList);
    }

    for (auto& msg : msgListFilterAgain) {
      const auto& tranMsg = msg->getProperty(MQMessageConst::PROPERTY_TRANSACTION_PREPARED);
      if (UtilAll::stob(tranMsg)) {
        msg->set_transaction_id(msg->getProperty(MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX));
      }
      MessageAccessor::putProperty(*msg, MQMessageConst::PROPERTY_MIN_OFFSET,
                                   UtilAll::to_string(pullResult.min_offset()));
      MessageAccessor::putProperty(*msg, MQMessageConst::PROPERTY_MAX_OFFSET,
                                   UtilAll::to_string(pullResult.max_offset()));
    }
  }

  return PullResult(pullResultExt.pull_status(), pullResultExt.next_begin_offset(), pullResultExt.min_offset(),
                    pullResultExt.max_offset(), std::move(msgListFilterAgain));
}

PullResult* PullAPIWrapper::pullKernelImpl(const MQMessageQueue& mq,             // 1
                                           const std::string& subExpression,     // 2
                                           int64_t subVersion,                   // 3
                                           int64_t offset,                       // 4
                                           int maxNums,                          // 5
                                           int sysFlag,                          // 6
                                           int64_t commitOffset,                 // 7
                                           int brokerSuspendMaxTimeMillis,       // 8
                                           int timeoutMillis,                    // 9
                                           CommunicationMode communicationMode,  // 10
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
