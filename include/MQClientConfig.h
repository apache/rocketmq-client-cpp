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
#ifndef __MQ_CLIENT_CONFIG_H__
#define __MQ_CLIENT_CONFIG_H__

#include "RPCHook.h"

namespace rocketmq {

/**
 * MQ Client Config
 */
class ROCKETMQCLIENT_API MQClientConfig {
 public:
  MQClientConfig();
  virtual ~MQClientConfig() = default;

  // clientId=processId-ipAddr@instanceName
  std::string buildMQClientId() const;

  // groupName
  const std::string& getGroupName() const;
  void setGroupName(const std::string& groupname);

  const std::string& getNamesrvAddr() const;
  void setNamesrvAddr(const std::string& namesrvAddr);

  const std::string& getInstanceName() const;
  void setInstanceName(const std::string& instanceName);

  void changeInstanceNameToPID();

  /**
   * set TcpTransport pull thread num, which dermine the num of threads to distribute network data,
   *
   *  1. its default value is CPU num, it must be setted before producer/consumer start, minimum value is CPU num;
   *  2. this pullThread num must be tested on your environment to find the best value for RT of sendMsg or delay time
   *     of consume msg before you change it;
   *  3. producer and consumer need different pullThread num, if set this num, producer and consumer must set different
   *     instanceName.
   *  4. configuration suggestion:
   *      1>. minimum RT of sendMsg:
   *              pullThreadNum = brokerNum*2
   **/
  int getTcpTransportWorkerThreadNum() const;
  void setTcpTransportWorkerThreadNum(int num);

  /**
   * timeout of tcp connect, it is same meaning for both producer and consumer;
   *  1. default value is 3000ms
   *  2. input parameter could only be milliSecond, suggestion value is 1000-3000ms;
   **/
  uint64_t getTcpTransportConnectTimeout() const;
  void setTcpTransportConnectTimeout(uint64_t timeout);  // ms

  /**
   * timeout of tryLock tcpTransport before sendMsg/pullMsg, if timeout, returns NULL
   *  1. paremeter unit is ms, default value is 3000ms, the minimun value is 1000ms, suggestion value is 3000ms;
   *  2. if configured with value smaller than 1000ms, the tryLockTimeout value will be setted to 1000ms
   **/
  uint64_t getTcpTransportTryLockTimeout() const;
  void setTcpTransportTryLockTimeout(uint64_t timeout);  // ms

  const std::string& getUnitName() const;
  void setUnitName(std::string unitName);

 protected:
  std::string m_namesrvAddr;
  std::string m_instanceName;
  std::string m_groupName;
  std::string m_unitName;

  int m_tcpWorkerThreadNum;
  uint64_t m_tcpConnectTimeout;           // ms
  uint64_t m_tcpTransportTryLockTimeout;  // s
};

}  // namespace rocketmq

#endif  // __MQ_CLIENT_CONFIG_H__
