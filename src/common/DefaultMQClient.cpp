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

#include "DefaultMQClient.h"
#include "Logging.h"
#include "MQClientFactory.h"
#include "MQClientManager.h"
#include "NameSpaceUtil.h"
#include "TopicPublishInfo.h"
#include "UtilAll.h"

namespace rocketmq {
// hard code first.
#define ROCKETMQCPP_VERSION "2.2.0"
#define BUILD_DATE "17:30:34 03-26-2020"
// display version: strings bin/librocketmq.so |grep VERSION
const char* rocketmq_build_time = "CPP CORE VERSION: " ROCKETMQCPP_VERSION ", BUILD TIME: " BUILD_DATE;

//<!************************************************************************
DefaultMQClient::DefaultMQClient() {
  string NAMESRV_ADDR_ENV = "NAMESRV_ADDR";
  if (const char* addr = getenv(NAMESRV_ADDR_ENV.c_str()))
    m_namesrvAddr = addr;
  else
    m_namesrvAddr = "";

  m_instanceName = "DEFAULT";
  m_nameSpace = "";
  m_clientFactory = NULL;
  m_serviceState = CREATE_JUST;
  m_pullThreadNum = std::thread::hardware_concurrency();
  m_tcpConnectTimeout = 3000;        // 3s
  m_tcpTransportTryLockTimeout = 3;  // 3s
  m_unitName = "";
  m_messageTrace = false;
}

DefaultMQClient::~DefaultMQClient() {}

string DefaultMQClient::getMQClientId() const {
  string clientIP = UtilAll::getLocalAddress();
  string processId = UtilAll::to_string(getpid());
  // return processId + "-" + clientIP + "@" + m_instanceName;
  return clientIP + "@" + processId + "#" + m_instanceName;
}
// version
string DefaultMQClient::getClientVersionString() const {
  string version(rocketmq_build_time);
  return version;
}

//<!groupName;
const string& DefaultMQClient::getGroupName() const {
  return m_GroupName;
}

void DefaultMQClient::setGroupName(const string& groupname) {
  m_GroupName = groupname;
}

const string& DefaultMQClient::getNamesrvAddr() const {
  return m_namesrvAddr;
}

void DefaultMQClient::setNamesrvAddr(const string& namesrvAddr) {
  m_namesrvAddr = NameSpaceUtil::formatNameServerURL(namesrvAddr);
}

const string& DefaultMQClient::getNamesrvDomain() const {
  return m_namesrvDomain;
}

void DefaultMQClient::setNamesrvDomain(const string& namesrvDomain) {
  m_namesrvDomain = namesrvDomain;
}

const string& DefaultMQClient::getInstanceName() const {
  return m_instanceName;
}

void DefaultMQClient::setInstanceName(const string& instanceName) {
  m_instanceName = instanceName;
}
const string& DefaultMQClient::getNameSpace() const {
  return m_nameSpace;
}

void DefaultMQClient::setNameSpace(const string& nameSpace) {
  m_nameSpace = nameSpace;
}
void DefaultMQClient::createTopic(const string& key, const string& newTopic, int queueNum) {
  try {
    getFactory()->createTopic(key, newTopic, queueNum, m_SessionCredentials);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
  }
}

int64 DefaultMQClient::earliestMsgStoreTime(const MQMessageQueue& mq) {
  return getFactory()->earliestMsgStoreTime(mq, m_SessionCredentials);
}

QueryResult DefaultMQClient::queryMessage(const string& topic, const string& key, int maxNum, int64 begin, int64 end) {
  return getFactory()->queryMessage(topic, key, maxNum, begin, end, m_SessionCredentials);
}

int64 DefaultMQClient::minOffset(const MQMessageQueue& mq) {
  return getFactory()->minOffset(mq, m_SessionCredentials);
}

int64 DefaultMQClient::maxOffset(const MQMessageQueue& mq) {
  return getFactory()->maxOffset(mq, m_SessionCredentials);
}

int64 DefaultMQClient::searchOffset(const MQMessageQueue& mq, uint64_t timestamp) {
  return getFactory()->searchOffset(mq, timestamp, m_SessionCredentials);
}

MQMessageExt* DefaultMQClient::viewMessage(const string& msgId) {
  return getFactory()->viewMessage(msgId, m_SessionCredentials);
}

vector<MQMessageQueue> DefaultMQClient::getTopicMessageQueueInfo(const string& topic) {
  boost::weak_ptr<TopicPublishInfo> weak_topicPublishInfo(
      getFactory()->tryToFindTopicPublishInfo(topic, m_SessionCredentials));
  boost::shared_ptr<TopicPublishInfo> topicPublishInfo(weak_topicPublishInfo.lock());
  if (topicPublishInfo) {
    return topicPublishInfo->getMessageQueueList();
  }
  THROW_MQEXCEPTION(MQClientException, "could not find MessageQueue Info of topic: [" + topic + "].", -1);
}

void DefaultMQClient::start() {
  if (getFactory() == NULL) {
    m_clientFactory = MQClientManager::getInstance()->getMQClientFactory(
        getMQClientId(), m_pullThreadNum, m_tcpConnectTimeout, m_tcpTransportTryLockTimeout, m_unitName, m_enableSsl,
        m_sslPropertyFile);
  }
  LOG_INFO(
      "MQClient "
      "start,groupname:%s,clientID:%s,instanceName:%s,nameserveraddr:%s",
      getGroupName().c_str(), getMQClientId().c_str(), getInstanceName().c_str(), getNamesrvAddr().c_str());
}

void DefaultMQClient::shutdown() {
  m_clientFactory->shutdown();
  m_clientFactory = NULL;
}

MQClientFactory* DefaultMQClient::getFactory() const {
  return m_clientFactory;
}
void DefaultMQClient::setFactory(MQClientFactory* factory) {
  m_clientFactory = factory;
}

bool DefaultMQClient::isServiceStateOk() {
  return m_serviceState == RUNNING;
}

void DefaultMQClient::setLogLevel(elogLevel inputLevel) {
  ALOG_ADAPTER->setLogLevel(inputLevel);
}

elogLevel DefaultMQClient::getLogLevel() {
  return ALOG_ADAPTER->getLogLevel();
}

void DefaultMQClient::setLogFileSizeAndNum(int fileNum, long perFileSize) {
  ALOG_ADAPTER->setLogFileNumAndSize(fileNum, perFileSize);
}

void DefaultMQClient::setTcpTransportPullThreadNum(int num) {
  if (num > m_pullThreadNum) {
    m_pullThreadNum = num;
  }
}

int DefaultMQClient::getTcpTransportPullThreadNum() const {
  return m_pullThreadNum;
}

void DefaultMQClient::setTcpTransportConnectTimeout(uint64_t timeout) {
  m_tcpConnectTimeout = timeout;
}
uint64_t DefaultMQClient::getTcpTransportConnectTimeout() const {
  return m_tcpConnectTimeout;
}

void DefaultMQClient::setTcpTransportTryLockTimeout(uint64_t timeout) {
  if (timeout < 1000) {
    timeout = 1000;
  }
  m_tcpTransportTryLockTimeout = timeout / 1000;
}
uint64_t DefaultMQClient::getTcpTransportTryLockTimeout() const {
  return m_tcpTransportTryLockTimeout;
}

void DefaultMQClient::setUnitName(const std::string& unitName) {
  m_unitName = unitName;
}
const string& DefaultMQClient::getUnitName() const {
  return m_unitName;
}

bool DefaultMQClient::getMessageTrace() const {
  return m_messageTrace;
}

void DefaultMQClient::setMessageTrace(bool mMessageTrace) {
  m_messageTrace = mMessageTrace;
}

void DefaultMQClient::setSessionCredentials(const string& input_accessKey,
                                            const string& input_secretKey,
                                            const string& input_onsChannel) {
  m_SessionCredentials.setAccessKey(input_accessKey);
  m_SessionCredentials.setSecretKey(input_secretKey);
  m_SessionCredentials.setAuthChannel(input_onsChannel);
}

const SessionCredentials& DefaultMQClient::getSessionCredentials() const {
  return m_SessionCredentials;
}

void DefaultMQClient::setEnableSsl(bool enableSsl) {
  m_enableSsl = enableSsl;
}

bool DefaultMQClient::getEnableSsl() const {
  return m_enableSsl;
}

void DefaultMQClient::setSslPropertyFile(const std::string& sslPropertyFile) {
  m_sslPropertyFile = sslPropertyFile;
}

const std::string& DefaultMQClient::getSslPropertyFile() const {
  return m_sslPropertyFile;
}

void DefaultMQClient::showClientConfigs() {
  // LOG_WARN("*****************************************************************************");
  LOG_WARN("ClientID:%s", getMQClientId().c_str());
  LOG_WARN("GroupName:%s", m_GroupName.c_str());
  LOG_WARN("NameServer:%s", m_namesrvAddr.c_str());
  LOG_WARN("NameServerDomain:%s", m_namesrvDomain.c_str());
  LOG_WARN("NameSpace:%s", m_nameSpace.c_str());
  LOG_WARN("InstanceName:%s", m_instanceName.c_str());
  LOG_WARN("UnitName:%s", m_unitName.c_str());
  LOG_WARN("PullThreadNum:%d", m_pullThreadNum);
  LOG_WARN("TcpConnectTimeout:%lld ms", m_tcpConnectTimeout);
  LOG_WARN("TcpTransportTryLockTimeout:%lld s", m_tcpTransportTryLockTimeout);
  LOG_WARN("EnableSsl:%s", m_enableSsl ? "true" : "false");
  LOG_WARN("SslPropertyFile:%s", m_sslPropertyFile.c_str());
  LOG_WARN("OpenMessageTrace:%s", m_messageTrace ? "true" : "false");
  // LOG_WARN("*****************************************************************************");
}
//<!************************************************************************
}  // namespace rocketmq
