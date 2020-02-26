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

#include "DefaultMQProducerImpl.h"

#include <assert.h>
#include <typeindex>

#include "BatchMessage.h"
#include "CommandHeader.h"
#include "CommunicationMode.h"
#include "Logging.h"
#include "MQClientAPIImpl.h"
#include "MQClientException.h"
#include "MQClientFactory.h"
#include "MQClientManager.h"
#include "MQDecoder.h"
#include "MQProtos.h"
#include "MessageAccessor.h"
#include "NameSpaceUtil.h"
#include "StringIdMaker.h"
#include "TopicPublishInfo.h"
#include "Validators.h"

namespace rocketmq {

//<!************************************************************************
DefaultMQProducerImpl::DefaultMQProducerImpl(const string& groupname)
    : m_sendMsgTimeout(3000),
      m_compressMsgBodyOverHowmuch(4 * 1024),
      m_maxMessageSize(1024 * 128),
      // m_retryAnotherBrokerWhenNotStoreOK(false),
      m_compressLevel(5),
      m_retryTimes(5),
      m_retryTimes4Async(1) {
  //<!set default group name;
  string gname = groupname.empty() ? DEFAULT_PRODUCER_GROUP : groupname;
  setGroupName(gname);
}

DefaultMQProducerImpl::~DefaultMQProducerImpl() {}

void DefaultMQProducerImpl::start() {
#ifndef WIN32
  /* Ignore the SIGPIPE */
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  sigaction(SIGPIPE, &sa, 0);
#endif
  LOG_INFO("###Current Producer@%s", getClientVersionString().c_str());
  // we should deal with namespaced before start.
  dealWithNameSpace();
  switch (m_serviceState) {
    case CREATE_JUST: {
      m_serviceState = START_FAILED;
      DefaultMQClient::start();
      LOG_INFO("DefaultMQProducerImpl:%s start", m_GroupName.c_str());

      bool registerOK = getFactory()->registerProducer(this);
      if (!registerOK) {
        m_serviceState = CREATE_JUST;
        THROW_MQEXCEPTION(
            MQClientException,
            "The producer group[" + getGroupName() + "] has been created before, specify another name please.", -1);
      }

      getFactory()->start();
      getFactory()->sendHeartbeatToAllBroker();
      m_serviceState = RUNNING;
      break;
    }
    case RUNNING:
    case START_FAILED:
    case SHUTDOWN_ALREADY:
      break;
    default:
      break;
  }
}

void DefaultMQProducerImpl::shutdown() {
  switch (m_serviceState) {
    case RUNNING: {
      LOG_INFO("DefaultMQProducerImpl shutdown");
      getFactory()->unregisterProducer(this);
      getFactory()->shutdown();
      m_serviceState = SHUTDOWN_ALREADY;
      break;
    }
    case SHUTDOWN_ALREADY:
    case CREATE_JUST:
      break;
    default:
      break;
  }
}

SendResult DefaultMQProducerImpl::send(MQMessage& msg, bool bSelectActiveBroker) {
  Validators::checkMessage(msg, getMaxMessageSize());
  if (!NameSpaceUtil::hasNameSpace(msg.getTopic(), getNameSpace())) {
    MessageAccessor::withNameSpace(msg, getNameSpace());
  }
  try {
    return sendDefaultImpl(msg, ComMode_SYNC, NULL, bSelectActiveBroker);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    throw e;
  }
  return SendResult();
}

void DefaultMQProducerImpl::send(MQMessage& msg, SendCallback* pSendCallback, bool bSelectActiveBroker) {
  Validators::checkMessage(msg, getMaxMessageSize());
  if (!NameSpaceUtil::hasNameSpace(msg.getTopic(), getNameSpace())) {
    MessageAccessor::withNameSpace(msg, getNameSpace());
  }
  try {
    sendDefaultImpl(msg, ComMode_ASYNC, pSendCallback, bSelectActiveBroker);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    throw e;
  }
}

SendResult DefaultMQProducerImpl::send(std::vector<MQMessage>& msgs) {
  SendResult result;
  try {
    BatchMessage batchMessage = buildBatchMessage(msgs);
    result = sendDefaultImpl(batchMessage, ComMode_SYNC, NULL);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    throw e;
  }
  return result;
}

SendResult DefaultMQProducerImpl::send(std::vector<MQMessage>& msgs, const MQMessageQueue& mq) {
  SendResult result;
  try {
    BatchMessage batchMessage = buildBatchMessage(msgs);
    result = sendKernelImpl(batchMessage, mq, ComMode_SYNC, NULL);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    throw e;
  }
  return result;
}

BatchMessage DefaultMQProducerImpl::buildBatchMessage(std::vector<MQMessage>& msgs) {
  if (msgs.size() < 1) {
    THROW_MQEXCEPTION(MQClientException, "msgs need one message at least", -1);
  }
  BatchMessage batchMessage;
  bool firstFlag = true;
  string topic;
  bool waitStoreMsgOK = false;
  for (auto& msg : msgs) {
    Validators::checkMessage(msg, getMaxMessageSize());
    if (!NameSpaceUtil::hasNameSpace(msg.getTopic(), getNameSpace())) {
      MessageAccessor::withNameSpace(msg, getNameSpace());
    }
    if (firstFlag) {
      topic = msg.getTopic();
      waitStoreMsgOK = msg.isWaitStoreMsgOK();
      firstFlag = false;

      if (UtilAll::startsWith_retry(topic)) {
        THROW_MQEXCEPTION(MQClientException, "Retry Group is not supported for batching", -1);
      }
    } else {
      if (msg.getDelayTimeLevel() > 0) {
        THROW_MQEXCEPTION(MQClientException, "TimeDelayLevel in not supported for batching", -1);
      }
      if (msg.getTopic() != topic) {
        THROW_MQEXCEPTION(MQClientException, "msgs need one message at least", -1);
      }
      if (msg.isWaitStoreMsgOK() != waitStoreMsgOK) {
        THROW_MQEXCEPTION(MQClientException, "msgs need one message at least", -2);
      }
    }
  }
  batchMessage.setBody(BatchMessage::encode(msgs));
  batchMessage.setTopic(topic);
  batchMessage.setWaitStoreMsgOK(waitStoreMsgOK);
  return batchMessage;
}

SendResult DefaultMQProducerImpl::send(MQMessage& msg, const MQMessageQueue& mq) {
  Validators::checkMessage(msg, getMaxMessageSize());
  if (!NameSpaceUtil::hasNameSpace(msg.getTopic(), getNameSpace())) {
    MessageAccessor::withNameSpace(msg, getNameSpace());
  }
  if (msg.getTopic() != mq.getTopic()) {
    LOG_WARN("message's topic not equal mq's topic");
  }
  try {
    return sendKernelImpl(msg, mq, ComMode_SYNC, NULL);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    throw e;
  }
  return SendResult();
}

void DefaultMQProducerImpl::send(MQMessage& msg, const MQMessageQueue& mq, SendCallback* pSendCallback) {
  Validators::checkMessage(msg, getMaxMessageSize());
  if (!NameSpaceUtil::hasNameSpace(msg.getTopic(), getNameSpace())) {
    MessageAccessor::withNameSpace(msg, getNameSpace());
  }
  if (msg.getTopic() != mq.getTopic()) {
    LOG_WARN("message's topic not equal mq's topic");
  }
  try {
    sendKernelImpl(msg, mq, ComMode_ASYNC, pSendCallback);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    throw e;
  }
}

void DefaultMQProducerImpl::sendOneway(MQMessage& msg, bool bSelectActiveBroker) {
  Validators::checkMessage(msg, getMaxMessageSize());
  if (!NameSpaceUtil::hasNameSpace(msg.getTopic(), getNameSpace())) {
    MessageAccessor::withNameSpace(msg, getNameSpace());
  }
  try {
    sendDefaultImpl(msg, ComMode_ONEWAY, NULL, bSelectActiveBroker);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    throw e;
  }
}

void DefaultMQProducerImpl::sendOneway(MQMessage& msg, const MQMessageQueue& mq) {
  Validators::checkMessage(msg, getMaxMessageSize());
  if (!NameSpaceUtil::hasNameSpace(msg.getTopic(), getNameSpace())) {
    MessageAccessor::withNameSpace(msg, getNameSpace());
  }
  if (msg.getTopic() != mq.getTopic()) {
    LOG_WARN("message's topic not equal mq's topic");
  }
  try {
    sendKernelImpl(msg, mq, ComMode_ONEWAY, NULL);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    throw e;
  }
}

SendResult DefaultMQProducerImpl::send(MQMessage& msg, MessageQueueSelector* pSelector, void* arg) {
  try {
    if (!NameSpaceUtil::hasNameSpace(msg.getTopic(), getNameSpace())) {
      MessageAccessor::withNameSpace(msg, getNameSpace());
    }
    return sendSelectImpl(msg, pSelector, arg, ComMode_SYNC, NULL);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    throw e;
  }
  return SendResult();
}

SendResult DefaultMQProducerImpl::send(MQMessage& msg,
                                       MessageQueueSelector* pSelector,
                                       void* arg,
                                       int autoRetryTimes,
                                       bool bActiveBroker) {
  try {
    if (!NameSpaceUtil::hasNameSpace(msg.getTopic(), getNameSpace())) {
      MessageAccessor::withNameSpace(msg, getNameSpace());
    }
    return sendAutoRetrySelectImpl(msg, pSelector, arg, ComMode_SYNC, NULL, autoRetryTimes, bActiveBroker);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    throw e;
  }
  return SendResult();
}

void DefaultMQProducerImpl::send(MQMessage& msg,
                                 MessageQueueSelector* pSelector,
                                 void* arg,
                                 SendCallback* pSendCallback) {
  try {
    if (!NameSpaceUtil::hasNameSpace(msg.getTopic(), getNameSpace())) {
      MessageAccessor::withNameSpace(msg, getNameSpace());
    }
    sendSelectImpl(msg, pSelector, arg, ComMode_ASYNC, pSendCallback);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    throw e;
  }
}

void DefaultMQProducerImpl::sendOneway(MQMessage& msg, MessageQueueSelector* pSelector, void* arg) {
  try {
    if (!NameSpaceUtil::hasNameSpace(msg.getTopic(), getNameSpace())) {
      MessageAccessor::withNameSpace(msg, getNameSpace());
    }
    sendSelectImpl(msg, pSelector, arg, ComMode_ONEWAY, NULL);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    throw e;
  }
}

int DefaultMQProducerImpl::getSendMsgTimeout() const {
  return m_sendMsgTimeout;
}

void DefaultMQProducerImpl::setSendMsgTimeout(int sendMsgTimeout) {
  m_sendMsgTimeout = sendMsgTimeout;
}

int DefaultMQProducerImpl::getCompressMsgBodyOverHowmuch() const {
  return m_compressMsgBodyOverHowmuch;
}

void DefaultMQProducerImpl::setCompressMsgBodyOverHowmuch(int compressMsgBodyOverHowmuch) {
  m_compressMsgBodyOverHowmuch = compressMsgBodyOverHowmuch;
}

int DefaultMQProducerImpl::getMaxMessageSize() const {
  return m_maxMessageSize;
}

void DefaultMQProducerImpl::setMaxMessageSize(int maxMessageSize) {
  m_maxMessageSize = maxMessageSize;
}

int DefaultMQProducerImpl::getCompressLevel() const {
  return m_compressLevel;
}

void DefaultMQProducerImpl::setCompressLevel(int compressLevel) {
  assert((compressLevel >= 0 && compressLevel <= 9) || compressLevel == -1);

  m_compressLevel = compressLevel;
}

//<!************************************************************************
SendResult DefaultMQProducerImpl::sendDefaultImpl(MQMessage& msg,
                                                  int communicationMode,
                                                  SendCallback* pSendCallback,
                                                  bool bActiveMQ) {
  MQMessageQueue lastmq;
  int mq_index = 0;
  for (int times = 1; times <= m_retryTimes; times++) {
    boost::weak_ptr<TopicPublishInfo> weak_topicPublishInfo(
        getFactory()->tryToFindTopicPublishInfo(msg.getTopic(), getSessionCredentials()));
    boost::shared_ptr<TopicPublishInfo> topicPublishInfo(weak_topicPublishInfo.lock());
    if (topicPublishInfo) {
      if (times == 1) {
        mq_index = topicPublishInfo->getWhichQueue();
      } else {
        mq_index++;
      }

      SendResult sendResult;
      MQMessageQueue mq;
      if (bActiveMQ)
        mq = topicPublishInfo->selectOneActiveMessageQueue(lastmq, mq_index);
      else
        mq = topicPublishInfo->selectOneMessageQueue(lastmq, mq_index);

      lastmq = mq;
      if (mq.getQueueId() == -1) {
        // THROW_MQEXCEPTION(MQClientException, "the MQMessageQueue is
        // invalide", -1);
        continue;
      }

      try {
        LOG_DEBUG("send to mq:%s", mq.toString().data());
        sendResult = sendKernelImpl(msg, mq, communicationMode, pSendCallback);
        switch (communicationMode) {
          case ComMode_ASYNC:
            return sendResult;
          case ComMode_ONEWAY:
            return sendResult;
          case ComMode_SYNC:
            if (sendResult.getSendStatus() != SEND_OK) {
              if (bActiveMQ) {
                topicPublishInfo->updateNonServiceMessageQueue(mq, getSendMsgTimeout());
              }
              continue;
            }
            return sendResult;
          default:
            break;
        }
      } catch (...) {
        LOG_ERROR("send failed of times:%d,brokerName:%s", times, mq.getBrokerName().c_str());
        if (bActiveMQ) {
          topicPublishInfo->updateNonServiceMessageQueue(mq, getSendMsgTimeout());
        }
        continue;
      }
    }  // end of for
    LOG_WARN("Retry many times, still failed");
  }
  string info = "No route info of this topic: " + msg.getTopic();
  THROW_MQEXCEPTION(MQClientException, info, -1);
}

SendResult DefaultMQProducerImpl::sendKernelImpl(MQMessage& msg,
                                                 const MQMessageQueue& mq,
                                                 int communicationMode,
                                                 SendCallback* sendCallback) {
  string brokerAddr = getFactory()->findBrokerAddressInPublish(mq.getBrokerName());

  if (brokerAddr.empty()) {
    getFactory()->tryToFindTopicPublishInfo(mq.getTopic(), getSessionCredentials());
    brokerAddr = getFactory()->findBrokerAddressInPublish(mq.getBrokerName());
  }

  if (!brokerAddr.empty()) {
    try {
      bool isBatchMsg = std::type_index(typeid(msg)) == std::type_index(typeid(BatchMessage));
      // msgId is produced by client, offsetMsgId produced by broker. (same with java sdk)
      if (!isBatchMsg) {
        string unique_id = StringIdMaker::getInstance().createUniqID();
        msg.setProperty(MQMessage::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX, unique_id);

        // batch does not support compressing right now,
        tryToCompressMessage(msg);
      }

      LOG_DEBUG("produce before:%s to %s", msg.toString().c_str(), mq.toString().c_str());

      SendMessageRequestHeader* requestHeader = new SendMessageRequestHeader();
      requestHeader->producerGroup = getGroupName();
      requestHeader->topic = (msg.getTopic());
      requestHeader->defaultTopic = DEFAULT_TOPIC;
      requestHeader->defaultTopicQueueNums = 4;
      requestHeader->queueId = (mq.getQueueId());
      requestHeader->sysFlag = (msg.getSysFlag());
      requestHeader->bornTimestamp = UtilAll::currentTimeMillis();
      requestHeader->flag = (msg.getFlag());
      requestHeader->consumeRetryTimes = 16;
      requestHeader->batch = isBatchMsg;
      requestHeader->properties = (MQDecoder::messageProperties2String(msg.getProperties()));

      return getFactory()->getMQClientAPIImpl()->sendMessage(brokerAddr, mq.getBrokerName(), msg, requestHeader,
                                                             getSendMsgTimeout(), getRetryTimes4Async(),
                                                             communicationMode, sendCallback, getSessionCredentials());
    } catch (MQException& e) {
      throw e;
    }
  }
  THROW_MQEXCEPTION(MQClientException, "The broker[" + mq.getBrokerName() + "] not exist", -1);
}

SendResult DefaultMQProducerImpl::sendSelectImpl(MQMessage& msg,
                                                 MessageQueueSelector* pSelector,
                                                 void* pArg,
                                                 int communicationMode,
                                                 SendCallback* sendCallback) {
  Validators::checkMessage(msg, getMaxMessageSize());

  boost::weak_ptr<TopicPublishInfo> weak_topicPublishInfo(
      getFactory()->tryToFindTopicPublishInfo(msg.getTopic(), getSessionCredentials()));
  boost::shared_ptr<TopicPublishInfo> topicPublishInfo(weak_topicPublishInfo.lock());
  if (topicPublishInfo)  //&& topicPublishInfo->ok())
  {
    MQMessageQueue mq = pSelector->select(topicPublishInfo->getMessageQueueList(), msg, pArg);
    return sendKernelImpl(msg, mq, communicationMode, sendCallback);
  }
  THROW_MQEXCEPTION(MQClientException, "No route info for this topic", -1);
}

SendResult DefaultMQProducerImpl::sendAutoRetrySelectImpl(MQMessage& msg,
                                                          MessageQueueSelector* pSelector,
                                                          void* pArg,
                                                          int communicationMode,
                                                          SendCallback* pSendCallback,
                                                          int autoRetryTimes,
                                                          bool bActiveMQ) {
  Validators::checkMessage(msg, getMaxMessageSize());

  MQMessageQueue lastmq;
  MQMessageQueue mq;
  int mq_index = 0;
  for (int times = 1; times <= autoRetryTimes + 1; times++) {
    boost::weak_ptr<TopicPublishInfo> weak_topicPublishInfo(
        getFactory()->tryToFindTopicPublishInfo(msg.getTopic(), getSessionCredentials()));
    boost::shared_ptr<TopicPublishInfo> topicPublishInfo(weak_topicPublishInfo.lock());
    if (topicPublishInfo) {
      SendResult sendResult;
      if (times == 1) {
        // always send to selected MQ firstly, evenif bActiveMQ was setted to true
        mq = pSelector->select(topicPublishInfo->getMessageQueueList(), msg, pArg);
        lastmq = mq;
      } else {
        LOG_INFO("sendAutoRetrySelectImpl with times:%d", times);
        std::vector<MQMessageQueue> mqs(topicPublishInfo->getMessageQueueList());
        for (size_t i = 0; i < mqs.size(); i++) {
          if (mqs[i] == lastmq)
            mq_index = i;
        }
        if (bActiveMQ)
          mq = topicPublishInfo->selectOneActiveMessageQueue(lastmq, mq_index);
        else
          mq = topicPublishInfo->selectOneMessageQueue(lastmq, mq_index);
        lastmq = mq;
        if (mq.getQueueId() == -1) {
          // THROW_MQEXCEPTION(MQClientException, "the MQMessageQueue is
          // invalide", -1);
          continue;
        }
      }

      try {
        LOG_DEBUG("send to broker:%s", mq.toString().c_str());
        sendResult = sendKernelImpl(msg, mq, communicationMode, pSendCallback);
        switch (communicationMode) {
          case ComMode_ASYNC:
            return sendResult;
          case ComMode_ONEWAY:
            return sendResult;
          case ComMode_SYNC:
            if (sendResult.getSendStatus() != SEND_OK) {
              if (bActiveMQ) {
                topicPublishInfo->updateNonServiceMessageQueue(mq, getSendMsgTimeout());
              }
              continue;
            }
            return sendResult;
          default:
            break;
        }
      } catch (...) {
        LOG_ERROR("send failed of times:%d,mq:%s", times, mq.toString().c_str());
        if (bActiveMQ) {
          topicPublishInfo->updateNonServiceMessageQueue(mq, getSendMsgTimeout());
        }
        continue;
      }
    }  // end of for
    LOG_WARN("Retry many times, still failed");
  }
  THROW_MQEXCEPTION(MQClientException, "No route info of this topic, ", -1);
}

bool DefaultMQProducerImpl::tryToCompressMessage(MQMessage& msg) {
  int sysFlag = msg.getSysFlag();
  if ((sysFlag & MessageSysFlag::CompressedFlag) == MessageSysFlag::CompressedFlag) {
    return true;
  }

  string body = msg.getBody();
  if ((int)body.length() >= getCompressMsgBodyOverHowmuch()) {
    string outBody;
    if (UtilAll::deflate(body, outBody, getCompressLevel())) {
      msg.setBody(outBody);
      msg.setSysFlag(sysFlag | MessageSysFlag::CompressedFlag);
      return true;
    }
  }

  return false;
}

int DefaultMQProducerImpl::getRetryTimes() const {
  return m_retryTimes;
}

void DefaultMQProducerImpl::setRetryTimes(int times) {
  if (times <= 0) {
    LOG_WARN("set retry times illegal, use default value:5");
    return;
  }

  if (times > 15) {
    LOG_WARN("set retry times illegal, use max value:15");
    m_retryTimes = 15;
    return;
  }
  LOG_WARN("set retry times to:%d", times);
  m_retryTimes = times;
}

int DefaultMQProducerImpl::getRetryTimes4Async() const {
  return m_retryTimes4Async;
}

void DefaultMQProducerImpl::setRetryTimes4Async(int times) {
  if (times <= 0) {
    LOG_WARN("set retry times illegal, use default value:1");
    m_retryTimes4Async = 1;
    return;
  }

  if (times > 15) {
    LOG_WARN("set retry times illegal, use max value:15");
    m_retryTimes4Async = 15;
    return;
  }
  LOG_INFO("set retry times to:%d", times);
  m_retryTimes4Async = times;
}

// we should deal with name space before producer start.
bool DefaultMQProducerImpl::dealWithNameSpace() {
  string ns = getNameSpace();
  if (ns.empty()) {
    string nsAddr = getNamesrvAddr();
    if (!NameSpaceUtil::checkNameSpaceExistInNameServer(nsAddr)) {
      return true;
    }
    ns = NameSpaceUtil::getNameSpaceFromNsURL(nsAddr);
    // reset namespace
    setNameSpace(ns);
  }
  // reset group name
  if (!NameSpaceUtil::hasNameSpace(getGroupName(), ns)) {
    string fullGID = NameSpaceUtil::withNameSpace(getGroupName(), ns);
    setGroupName(fullGID);
  }
  return true;
}
//<!***************************************************************************
}  // namespace rocketmq
