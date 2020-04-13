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

#include <cassert>
#include <typeindex>

#ifndef WIN32
#include <signal.h>
#endif

#include "CommandHeader.h"
#include "CommunicationMode.h"
#include "Logging.h"
#include "MQClientAPIImpl.h"
#include "MQClientException.h"
#include "MQClientInstance.h"
#include "MQClientManager.h"
#include "MQDecoder.h"
#include "MQFaultStrategy.h"
#include "MQProtos.h"
#include "MessageBatch.h"
#include "MessageClientIDSetter.h"
#include "MessageSysFlag.h"
#include "TopicPublishInfo.h"
#include "TransactionMQProducer.h"
#include "Validators.h"

namespace rocketmq {

DefaultMQProducerImpl::DefaultMQProducerImpl(DefaultMQProducerConfigPtr config)
    : DefaultMQProducerImpl(config, nullptr) {}

DefaultMQProducerImpl::DefaultMQProducerImpl(DefaultMQProducerConfigPtr config, RPCHookPtr rpcHook)
    : MQClientImpl(config, rpcHook),
      m_producerConfig(config),
      m_mqFaultStrategy(new MQFaultStrategy()),
      m_checkTransactionExecutor(nullptr) {}

DefaultMQProducerImpl::~DefaultMQProducerImpl() = default;

void DefaultMQProducerImpl::start() {
#ifndef WIN32
  /* Ignore the SIGPIPE */
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  ::sigaction(SIGPIPE, &sa, 0);
#endif

  switch (m_serviceState) {
    case CREATE_JUST: {
      m_serviceState = START_FAILED;

      m_producerConfig->changeInstanceNameToPID();

      LOG_INFO_NEW("DefaultMQProducerImpl:{} start", m_producerConfig->getGroupName());

      MQClientImpl::start();

      bool registerOK = m_clientInstance->registerProducer(m_producerConfig->getGroupName(), this);
      if (!registerOK) {
        m_serviceState = CREATE_JUST;
        THROW_MQEXCEPTION(MQClientException,
                          "The producer group[" + m_producerConfig->getGroupName() +
                              "] has been created before, specify another name please.",
                          -1);
      }

      m_clientInstance->start();
      LOG_INFO_NEW("the producer [{}] start OK.", m_producerConfig->getGroupName());
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

  m_clientInstance->sendHeartbeatToAllBrokerWithLock();
}

void DefaultMQProducerImpl::shutdown() {
  switch (m_serviceState) {
    case RUNNING: {
      LOG_INFO("DefaultMQProducerImpl shutdown");
      m_clientInstance->unregisterProducer(m_producerConfig->getGroupName());
      m_clientInstance->shutdown();

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

void DefaultMQProducerImpl::initTransactionEnv() {
  if (nullptr == m_checkTransactionExecutor) {
    m_checkTransactionExecutor.reset(new thread_pool_executor(1, false));
  }
  m_checkTransactionExecutor->startup();
}

void DefaultMQProducerImpl::destroyTransactionEnv() {
  m_checkTransactionExecutor->shutdown();
}

SendResult DefaultMQProducerImpl::send(MQMessagePtr msg) {
  return send(msg, m_producerConfig->getSendMsgTimeout());
}

SendResult DefaultMQProducerImpl::send(MQMessagePtr msg, long timeout) {
  try {
    std::unique_ptr<SendResult> sendResult(sendDefaultImpl(msg, ComMode_SYNC, nullptr, timeout));
    return *sendResult;
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    throw e;
  }
}

SendResult DefaultMQProducerImpl::send(MQMessagePtr msg, const MQMessageQueue& mq) {
  return send(msg, mq, m_producerConfig->getSendMsgTimeout());
}

SendResult DefaultMQProducerImpl::send(MQMessagePtr msg, const MQMessageQueue& mq, long timeout) {
  Validators::checkMessage(*msg, m_producerConfig->getMaxMessageSize());

  if (msg->getTopic() != mq.getTopic()) {
    THROW_MQEXCEPTION(MQClientException, "message's topic not equal mq's topic", -1);
  }

  try {
    std::unique_ptr<SendResult> sendResult(sendKernelImpl(msg, mq, ComMode_SYNC, nullptr, nullptr, timeout));
    return *sendResult;
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    throw e;
  }
}

void DefaultMQProducerImpl::send(MQMessagePtr msg, SendCallback* sendCallback) noexcept {
  return send(msg, sendCallback, m_producerConfig->getSendMsgTimeout());
}

void DefaultMQProducerImpl::send(MQMessagePtr msg, SendCallback* sendCallback, long timeout) noexcept {
  try {
    (void)sendDefaultImpl(msg, ComMode_ASYNC, sendCallback, timeout);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    sendCallback->onException(e);
    if (sendCallback->getSendCallbackType() == SEND_CALLBACK_TYPE_ATUO_DELETE) {
      deleteAndZero(sendCallback);
    }
  } catch (std::exception& e) {
    LOG_FATAL_NEW("[BUG] encounter unexcepted exception: {}", e.what());
    exit(-1);
  }
}

void DefaultMQProducerImpl::send(MQMessagePtr msg, const MQMessageQueue& mq, SendCallback* sendCallback) noexcept {
  return send(msg, mq, sendCallback, m_producerConfig->getSendMsgTimeout());
}

void DefaultMQProducerImpl::send(MQMessagePtr msg,
                                 const MQMessageQueue& mq,
                                 SendCallback* sendCallback,
                                 long timeout) noexcept {
  try {
    Validators::checkMessage(*msg, m_producerConfig->getMaxMessageSize());

    if (msg->getTopic() != mq.getTopic()) {
      THROW_MQEXCEPTION(MQClientException, "message's topic not equal mq's topic", -1);
    }

    try {
      sendKernelImpl(msg, mq, ComMode_ASYNC, sendCallback, nullptr, timeout);
    } catch (MQBrokerException& e) {
      std::string info = std::string("unknown exception, ") + e.what();
      THROW_MQEXCEPTION(MQClientException, info, e.GetError());
    }
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    sendCallback->onException(e);
    if (sendCallback->getSendCallbackType() == SEND_CALLBACK_TYPE_ATUO_DELETE) {
      deleteAndZero(sendCallback);
    }
  } catch (std::exception& e) {
    LOG_FATAL_NEW("[BUG] encounter unexcepted exception: {}", e.what());
    exit(-1);
  }
}

void DefaultMQProducerImpl::sendOneway(MQMessagePtr msg) {
  try {
    sendDefaultImpl(msg, ComMode_ONEWAY, nullptr, m_producerConfig->getSendMsgTimeout());
  } catch (MQBrokerException e) {
    std::string info = std::string("unknown exception, ") + e.what();
    THROW_MQEXCEPTION(MQClientException, info, e.GetError());
  }
}

void DefaultMQProducerImpl::sendOneway(MQMessagePtr msg, const MQMessageQueue& mq) {
  Validators::checkMessage(*msg, m_producerConfig->getMaxMessageSize());

  if (msg->getTopic() != mq.getTopic()) {
    THROW_MQEXCEPTION(MQClientException, "message's topic not equal mq's topic", -1);
  }

  try {
    sendKernelImpl(msg, mq, ComMode_ONEWAY, nullptr, nullptr, m_producerConfig->getSendMsgTimeout());
  } catch (MQBrokerException e) {
    std::string info = std::string("unknown exception, ") + e.what();
    THROW_MQEXCEPTION(MQClientException, info, e.GetError());
  }
}

SendResult DefaultMQProducerImpl::send(MQMessagePtr msg, MessageQueueSelector* selector, void* arg) {
  return send(msg, selector, arg, m_producerConfig->getSendMsgTimeout());
}

SendResult DefaultMQProducerImpl::send(MQMessagePtr msg, MessageQueueSelector* selector, void* arg, long timeout) {
  try {
    std::unique_ptr<SendResult> result(sendSelectImpl(msg, selector, arg, ComMode_SYNC, nullptr, timeout));
    return *result.get();
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    throw e;
  }
}

void DefaultMQProducerImpl::send(MQMessagePtr msg,
                                 MessageQueueSelector* selector,
                                 void* arg,
                                 SendCallback* sendCallback) noexcept {
  return send(msg, selector, arg, sendCallback, m_producerConfig->getSendMsgTimeout());
}

void DefaultMQProducerImpl::send(MQMessagePtr msg,
                                 MessageQueueSelector* selector,
                                 void* arg,
                                 SendCallback* sendCallback,
                                 long timeout) noexcept {
  try {
    try {
      sendSelectImpl(msg, selector, arg, ComMode_ASYNC, sendCallback, timeout);
    } catch (MQBrokerException& e) {
      std::string info = std::string("unknown exception, ") + e.what();
      THROW_MQEXCEPTION(MQClientException, info, e.GetError());
    }
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    sendCallback->onException(e);
    if (sendCallback->getSendCallbackType() == SEND_CALLBACK_TYPE_ATUO_DELETE) {
      deleteAndZero(sendCallback);
    }
  } catch (std::exception& e) {
    LOG_FATAL_NEW("[BUG] encounter unexcepted exception: {}", e.what());
    exit(-1);
  }
}

void DefaultMQProducerImpl::sendOneway(MQMessagePtr msg, MessageQueueSelector* selector, void* arg) {
  try {
    sendSelectImpl(msg, selector, arg, ComMode_ONEWAY, nullptr, m_producerConfig->getSendMsgTimeout());
  } catch (MQBrokerException e) {
    std::string info = std::string("unknown exception, ") + e.what();
    THROW_MQEXCEPTION(MQClientException, info, e.GetError());
  }
}

TransactionSendResult DefaultMQProducerImpl::sendMessageInTransaction(MQMessagePtr msg, void* arg) {
  try {
    std::unique_ptr<TransactionSendResult> sendResult(
        sendMessageInTransactionImpl(msg, arg, m_producerConfig->getSendMsgTimeout()));
    return *sendResult;
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    throw e;
  }
}

SendResult DefaultMQProducerImpl::send(std::vector<MQMessagePtr>& msgs) {
  std::unique_ptr<MessageBatch> batchMessage(batch(msgs));
  return send(batchMessage.get());
}

SendResult DefaultMQProducerImpl::send(std::vector<MQMessagePtr>& msgs, long timeout) {
  std::unique_ptr<MessageBatch> batchMessage(batch(msgs));
  return send(batchMessage.get(), timeout);
}

SendResult DefaultMQProducerImpl::send(std::vector<MQMessagePtr>& msgs, const MQMessageQueue& mq) {
  std::unique_ptr<MessageBatch> batchMessage(batch(msgs));
  return send(batchMessage.get(), mq);
}

SendResult DefaultMQProducerImpl::send(std::vector<MQMessagePtr>& msgs, const MQMessageQueue& mq, long timeout) {
  std::unique_ptr<MessageBatch> batchMessage(batch(msgs));
  return send(batchMessage.get(), mq, timeout);
}

MessageBatch* DefaultMQProducerImpl::batch(std::vector<MQMessagePtr>& msgs) {
  if (msgs.size() < 1) {
    THROW_MQEXCEPTION(MQClientException, "msgs need one message at least", -1);
  }

  try {
    MessageBatch* msgBatch = MessageBatch::generateFromList(msgs);
    for (auto& message : msgBatch->getMessages()) {
      Validators::checkMessage(*message, m_producerConfig->getMaxMessageSize());
      MessageClientIDSetter::setUniqID(*message);
    }
    msgBatch->setBody(msgBatch->encode());
    return msgBatch;
  } catch (std::exception& e) {
    THROW_MQEXCEPTION(MQClientException, "Failed to initiate the MessageBatch", -1);
  }
}

const MQMessageQueue& DefaultMQProducerImpl::selectOneMessageQueue(TopicPublishInfo* tpInfo,
                                                                   const std::string& lastBrokerName) {
  return m_mqFaultStrategy->selectOneMessageQueue(tpInfo, lastBrokerName);
}

void DefaultMQProducerImpl::updateFaultItem(const std::string& brokerName, const long currentLatency, bool isolation) {
  m_mqFaultStrategy->updateFaultItem(brokerName, currentLatency, isolation);
}

SendResult* DefaultMQProducerImpl::sendDefaultImpl(MQMessagePtr msg,
                                                   CommunicationMode communicationMode,
                                                   SendCallback* sendCallback,
                                                   long timeout) {
  Validators::checkMessage(*msg, m_producerConfig->getMaxMessageSize());

  uint64_t beginTimestampFirst = UtilAll::currentTimeMillis();
  uint64_t beginTimestampPrev = beginTimestampFirst;
  uint64_t endTimestamp = beginTimestampFirst;
  TopicPublishInfoPtr topicPublishInfo = m_clientInstance->tryToFindTopicPublishInfo(msg->getTopic());
  if (topicPublishInfo != nullptr && topicPublishInfo->ok()) {
    bool callTimeout = false;
    std::unique_ptr<SendResult> sendResult;
    int timesTotal = communicationMode == CommunicationMode::ComMode_SYNC ? 1 + m_producerConfig->getRetryTimes() : 1;
    int times = 0;
    std::string lastBrokerName;
    for (; times < timesTotal; times++) {
      const auto& mq = selectOneMessageQueue(topicPublishInfo.get(), lastBrokerName);
      lastBrokerName = mq.getBrokerName();

      try {
        LOG_DEBUG("send to mq:%s", mq.toString().data());

        beginTimestampPrev = UtilAll::currentTimeMillis();
        if (times > 0) {
          // TODO: Reset topic with namespace during resend.
        }
        long costTime = beginTimestampPrev - beginTimestampFirst;
        if (timeout < costTime) {
          callTimeout = true;
          break;
        }

        sendResult.reset(
            sendKernelImpl(msg, mq, communicationMode, sendCallback, topicPublishInfo, timeout - costTime));
        endTimestamp = UtilAll::currentTimeMillis();
        updateFaultItem(mq.getBrokerName(), endTimestamp - beginTimestampPrev, false);
        switch (communicationMode) {
          case ComMode_ASYNC:
            return nullptr;
          case ComMode_ONEWAY:
            return nullptr;
          case ComMode_SYNC:
            if (sendResult->getSendStatus() != SEND_OK) {
              if (m_producerConfig->isRetryAnotherBrokerWhenNotStoreOK()) {
                continue;
              }
            }

            return sendResult.release();
          default:
            break;
        }
      } catch (std::exception& e) {
        // TODO: 区分异常类型
        endTimestamp = UtilAll::currentTimeMillis();
        updateFaultItem(mq.getBrokerName(), endTimestamp - beginTimestampPrev, true);
        LOG_ERROR_NEW("send failed of times:{}, brokerName:{}. exception:{}", times, mq.getBrokerName(), e.what());
        continue;
      }

    }  // end of for

    if (sendResult != nullptr) {
      return sendResult.release();
    }

    std::string info = "Send [" + UtilAll::to_string(times) + "] times, still failed, cost [" +
                       UtilAll::to_string(UtilAll::currentTimeMillis() - beginTimestampFirst) +
                       "]ms, Topic: " + msg->getTopic();
    THROW_MQEXCEPTION(MQClientException, info, -1);
  }

  THROW_MQEXCEPTION(MQClientException, "No route info of this topic: " + msg->getTopic(), -1);
}

SendResult* DefaultMQProducerImpl::sendKernelImpl(MQMessagePtr msg,
                                                  const MQMessageQueue& mq,
                                                  CommunicationMode communicationMode,
                                                  SendCallback* sendCallback,
                                                  TopicPublishInfoPtr topicPublishInfo,
                                                  long timeout) {
  uint64_t beginStartTime = UtilAll::currentTimeMillis();
  std::string brokerAddr = m_clientInstance->findBrokerAddressInPublish(mq.getBrokerName());
  if (brokerAddr.empty()) {
    m_clientInstance->tryToFindTopicPublishInfo(mq.getTopic());
    brokerAddr = m_clientInstance->findBrokerAddressInPublish(mq.getBrokerName());
  }

  if (!brokerAddr.empty()) {
    try {
      // for MessageBatch, ID has been set in the generating process
      if (!msg->isBatch()) {
        // msgId is produced by client, offsetMsgId produced by broker. (same with java sdk)
        MessageClientIDSetter::setUniqID(*msg);
      }

      int sysFlag = 0;
      bool msgBodyCompressed = false;
      if (tryToCompressMessage(*msg)) {
        sysFlag |= MessageSysFlag::CompressedFlag;
        msgBodyCompressed = true;
      }

      const auto& tranMsg = msg->getProperty(MQMessageConst::PROPERTY_TRANSACTION_PREPARED);
      if (UtilAll::stob(tranMsg)) {
        sysFlag |= MessageSysFlag::TransactionPreparedType;
      }

      // TOOD: send message hook

      std::unique_ptr<SendMessageRequestHeader> requestHeader(new SendMessageRequestHeader());
      requestHeader->producerGroup = m_producerConfig->getGroupName();
      requestHeader->topic = msg->getTopic();
      requestHeader->defaultTopic = AUTO_CREATE_TOPIC_KEY_TOPIC;
      requestHeader->defaultTopicQueueNums = 4;
      requestHeader->queueId = mq.getQueueId();
      requestHeader->sysFlag = sysFlag;
      requestHeader->bornTimestamp = UtilAll::currentTimeMillis();
      requestHeader->flag = msg->getFlag();
      requestHeader->properties = MQDecoder::messageProperties2String(msg->getProperties());
      requestHeader->reconsumeTimes = 0;
      requestHeader->unitMode = false;
      requestHeader->batch = msg->isBatch();

      if (UtilAll::isRetryTopic(mq.getTopic())) {
        const auto& reconsumeTimes = MessageAccessor::getReconsumeTime(*msg);
        if (!reconsumeTimes.empty()) {
          requestHeader->reconsumeTimes = std::stoi(reconsumeTimes);
          MessageAccessor::clearProperty(*msg, MQMessageConst::PROPERTY_RECONSUME_TIME);
        }

        const auto& maxReconsumeTimes = MessageAccessor::getMaxReconsumeTimes(*msg);
        if (!maxReconsumeTimes.empty()) {
          requestHeader->maxReconsumeTimes = std::stoi(maxReconsumeTimes);
          MessageAccessor::clearProperty(*msg, MQMessageConst::PROPERTY_MAX_RECONSUME_TIMES);
        }
      }

      SendResult* sendResult = nullptr;
      switch (communicationMode) {
        case ComMode_ASYNC: {
          long costTimeAsync = UtilAll::currentTimeMillis() - beginStartTime;
          if (timeout < costTimeAsync) {
            THROW_MQEXCEPTION(RemotingTooMuchRequestException, "sendKernelImpl call timeout", -1);
          }
          sendResult = m_clientInstance->getMQClientAPIImpl()->sendMessage(
              brokerAddr, mq.getBrokerName(), msg, std::move(requestHeader), timeout, communicationMode, sendCallback,
              topicPublishInfo, m_clientInstance, m_producerConfig->getRetryTimes4Async(), shared_from_this());
        } break;
        case ComMode_ONEWAY:
        case ComMode_SYNC: {
          long costTimeSync = UtilAll::currentTimeMillis() - beginStartTime;
          if (timeout < costTimeSync) {
            THROW_MQEXCEPTION(RemotingTooMuchRequestException, "sendKernelImpl call timeout", -1);
          }
          sendResult = m_clientInstance->getMQClientAPIImpl()->sendMessage(brokerAddr, mq.getBrokerName(), msg,
                                                                           std::move(requestHeader), timeout,
                                                                           communicationMode, shared_from_this());
        } break;
        default:
          assert(false);
          break;
      }

      return sendResult;
    } catch (MQException& e) {
      throw e;
    }
  }

  THROW_MQEXCEPTION(MQClientException, "The broker[" + mq.getBrokerName() + "] not exist", -1);
}

SendResult* DefaultMQProducerImpl::sendSelectImpl(MQMessagePtr msg,
                                                  MessageQueueSelector* selector,
                                                  void* arg,
                                                  CommunicationMode communicationMode,
                                                  SendCallback* sendCallback,
                                                  long timeout) {
  auto beginStartTime = UtilAll::currentTimeMillis();
  Validators::checkMessage(*msg, m_producerConfig->getMaxMessageSize());

  TopicPublishInfoPtr topicPublishInfo = m_clientInstance->tryToFindTopicPublishInfo(msg->getTopic());
  if (topicPublishInfo != nullptr && topicPublishInfo->ok()) {
    MQMessageQueue mq = selector->select(topicPublishInfo->getMessageQueueList(), *msg, arg);

    auto costTime = UtilAll::currentTimeMillis() - beginStartTime;
    if (timeout < costTime) {
      THROW_MQEXCEPTION(RemotingTooMuchRequestException, "sendSelectImpl call timeout", -1);
    }

    return sendKernelImpl(msg, mq, communicationMode, sendCallback, nullptr, timeout - costTime);
  }

  std::string info = std::string("No route info for this topic, ") + msg->getTopic();
  THROW_MQEXCEPTION(MQClientException, info, -1);
}

TransactionSendResult* DefaultMQProducerImpl::sendMessageInTransactionImpl(MQMessagePtr msg, void* arg, long timeout) {
  auto* transactionListener = getCheckListener();
  if (nullptr == transactionListener) {
    THROW_MQEXCEPTION(MQClientException, "transactionListener is null", -1);
  }

  std::unique_ptr<SendResult> sendResult;
  MessageAccessor::putProperty(*msg, MQMessageConst::PROPERTY_TRANSACTION_PREPARED, "true");
  MessageAccessor::putProperty(*msg, MQMessageConst::PROPERTY_PRODUCER_GROUP, m_producerConfig->getGroupName());
  try {
    sendResult.reset(sendDefaultImpl(msg, ComMode_SYNC, nullptr, timeout));
  } catch (MQException& e) {
    THROW_MQEXCEPTION(MQClientException, "send message Exception", -1);
  }

  LocalTransactionState localTransactionState = LocalTransactionState::UNKNOWN;
  std::exception_ptr localException;
  switch (sendResult->getSendStatus()) {
    case SendStatus::SEND_OK:
      try {
        if (!sendResult->getTransactionId().empty()) {
          msg->putProperty("__transactionId__", sendResult->getTransactionId());
        }
        const auto& transactionId = msg->getProperty(MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX);
        if (!transactionId.empty()) {
          msg->setTransactionId(transactionId);
        }
        localTransactionState = transactionListener->executeLocalTransaction(*msg, arg);
        if (localTransactionState != LocalTransactionState::COMMIT_MESSAGE) {
          LOG_INFO_NEW("executeLocalTransaction return not COMMIT_MESSAGE, msg:{}", msg->toString());
        }
      } catch (MQException& e) {
        LOG_INFO_NEW("executeLocalTransaction exception, msg:{}", msg->toString());
        localException = std::current_exception();
      }
      break;
    case SendStatus::SEND_FLUSH_DISK_TIMEOUT:
    case SendStatus::SEND_FLUSH_SLAVE_TIMEOUT:
    case SendStatus::SEND_SLAVE_NOT_AVAILABLE:
      localTransactionState = LocalTransactionState::ROLLBACK_MESSAGE;
      LOG_WARN_NEW("sendMessageInTransaction, send not ok, rollback, result:{}", sendResult->toString());
      break;
    default:
      break;
  }

  try {
    endTransaction(*sendResult, localTransactionState, localException);
  } catch (MQException& e) {
    LOG_WARN_NEW("local transaction execute {}, but end broker transaction failed: {}", localTransactionState,
                 e.what());
  }

  // FIXME: setTransactionId will cause OOM?
  TransactionSendResult* transactionSendResult = new TransactionSendResult(*sendResult.get());
  transactionSendResult->setTransactionId(msg->getTransactionId());
  transactionSendResult->setLocalTransactionState(localTransactionState);
  return transactionSendResult;
}

TransactionListener* DefaultMQProducerImpl::getCheckListener() {
  auto transactionProducerConfig = std::dynamic_pointer_cast<TransactionMQProducerConfig>(m_producerConfig);
  if (transactionProducerConfig != nullptr) {
    return transactionProducerConfig->getTransactionListener();
  }
  return nullptr;
};

void DefaultMQProducerImpl::checkTransactionState(const std::string& addr,
                                                  MQMessageExtPtr2 msg,
                                                  CheckTransactionStateRequestHeader* checkRequestHeader) {
  long tranStateTableOffset = checkRequestHeader->tranStateTableOffset;
  long commitLogOffset = checkRequestHeader->commitLogOffset;
  std::string msgId = checkRequestHeader->msgId;
  std::string transactionId = checkRequestHeader->transactionId;
  std::string offsetMsgId = checkRequestHeader->offsetMsgId;

  m_checkTransactionExecutor->submit(std::bind(&DefaultMQProducerImpl::checkTransactionStateImpl, this, addr, msg,
                                               tranStateTableOffset, commitLogOffset, msgId, transactionId,
                                               offsetMsgId));
}

void DefaultMQProducerImpl::checkTransactionStateImpl(const std::string& addr,
                                                      MQMessageExtPtr2 message,
                                                      long tranStateTableOffset,
                                                      long commitLogOffset,
                                                      const std::string& msgId,
                                                      const std::string& transactionId,
                                                      const std::string& offsetMsgId) {
  auto* transactionCheckListener = getCheckListener();
  if (nullptr == transactionCheckListener) {
    LOG_WARN_NEW("CheckTransactionState, pick transactionCheckListener by group[{}] failed",
                 m_producerConfig->getGroupName());
    return;
  }

  LocalTransactionState localTransactionState = UNKNOWN;
  std::exception_ptr exception;
  try {
    localTransactionState = transactionCheckListener->checkLocalTransaction(*message);
  } catch (MQException& e) {
    LOG_ERROR_NEW("Broker call checkTransactionState, but checkLocalTransactionState exception, {}", e.what());
    exception = std::current_exception();
  }

  EndTransactionRequestHeader* endHeader = new EndTransactionRequestHeader();
  endHeader->commitLogOffset = commitLogOffset;
  endHeader->producerGroup = m_producerConfig->getGroupName();
  endHeader->tranStateTableOffset = tranStateTableOffset;
  endHeader->fromTransactionCheck = true;

  std::string uniqueKey = message->getProperty(MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX);
  if (uniqueKey.empty()) {
    uniqueKey = message->getMsgId();
  }

  endHeader->msgId = uniqueKey;
  endHeader->transactionId = transactionId;
  switch (localTransactionState) {
    case COMMIT_MESSAGE:
      endHeader->commitOrRollback = MessageSysFlag::TransactionCommitType;
      break;
    case ROLLBACK_MESSAGE:
      endHeader->commitOrRollback = MessageSysFlag::TransactionRollbackType;
      LOG_WARN_NEW("when broker check, client rollback this transaction, {}", endHeader->toString());
      break;
    case UNKNOWN:
      endHeader->commitOrRollback = MessageSysFlag::TransactionNotType;
      LOG_WARN_NEW("when broker check, client does not know this transaction state, {}", endHeader->toString());
      break;
    default:
      break;
  }

  std::string remark;
  if (exception) {
    remark = "checkLocalTransactionState Exception: " + UtilAll::to_string(exception);
  }

  try {
    m_clientInstance->getMQClientAPIImpl()->endTransactionOneway(addr, endHeader, remark);
  } catch (std::exception& e) {
    LOG_ERROR_NEW("endTransactionOneway exception: {}", e.what());
  }
}

void DefaultMQProducerImpl::endTransaction(SendResult& sendResult,
                                           LocalTransactionState localTransactionState,
                                           std::exception_ptr& localException) {
  MQMessageId id;
  if (!sendResult.getOffsetMsgId().empty()) {
    id = MQDecoder::decodeMessageId(sendResult.getOffsetMsgId());
  } else {
    id = MQDecoder::decodeMessageId(sendResult.getMsgId());
  }
  const auto& transactionId = sendResult.getTransactionId();
  std::string brokerAddr = m_clientInstance->findBrokerAddressInPublish(sendResult.getMessageQueue().getBrokerName());
  EndTransactionRequestHeader* requestHeader = new EndTransactionRequestHeader();
  requestHeader->transactionId = transactionId;
  requestHeader->commitLogOffset = id.getOffset();
  switch (localTransactionState) {
    case COMMIT_MESSAGE:
      requestHeader->commitOrRollback = MessageSysFlag::TransactionCommitType;
      break;
    case ROLLBACK_MESSAGE:
      requestHeader->commitOrRollback = MessageSysFlag::TransactionRollbackType;
      break;
    case UNKNOWN:
      requestHeader->commitOrRollback = MessageSysFlag::TransactionNotType;
      break;
    default:
      break;
  }

  requestHeader->producerGroup = m_producerConfig->getGroupName();
  requestHeader->tranStateTableOffset = sendResult.getQueueOffset();
  requestHeader->msgId = sendResult.getMsgId();

  std::string remark =
      localException ? ("executeLocalTransactionBranch exception: " + UtilAll::to_string(localException)) : null;

  m_clientInstance->getMQClientAPIImpl()->endTransactionOneway(brokerAddr, requestHeader, remark);
}

bool DefaultMQProducerImpl::tryToCompressMessage(MQMessage& msg) {
  if (msg.isBatch()) {
    // batch dose not support compressing right now
    return false;
  }

  // already compressed
  if (UtilAll::stob(msg.getProperty(MQMessageConst::PROPERTY_ALREADY_COMPRESSED_FLAG))) {
    return true;
  }

  const auto& body = msg.getBody();
  if (body.length() >= m_producerConfig->getCompressMsgBodyOverHowmuch()) {
    std::string outBody;
    if (UtilAll::deflate(body, outBody, m_producerConfig->getCompressLevel())) {
      msg.setBody(std::move(outBody));
      msg.putProperty(MQMessageConst::PROPERTY_ALREADY_COMPRESSED_FLAG, "true");
      return true;
    }
  }

  return false;
}

bool DefaultMQProducerImpl::isSendLatencyFaultEnable() const {
  return m_mqFaultStrategy->isSendLatencyFaultEnable();
}

void DefaultMQProducerImpl::setSendLatencyFaultEnable(bool sendLatencyFaultEnable) {
  m_mqFaultStrategy->setSendLatencyFaultEnable(sendLatencyFaultEnable);
}

}  // namespace rocketmq
