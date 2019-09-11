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
#ifndef __CLIENT_REMOTING_PROCESSOR_H__
#define __CLIENT_REMOTING_PROCESSOR_H__

#include "MQMessageQueue.h"
#include "RemotingCommand.h"
#include "RequestProcessor.h"

namespace rocketmq {

class MQClientInstance;

class ClientRemotingProcessor : public RequestProcessor {
 public:
  ClientRemotingProcessor(MQClientInstance* mqClientFactory);
  virtual ~ClientRemotingProcessor();

  RemotingCommand* processRequest(const std::string& addr, RemotingCommand* request) override;

  RemotingCommand* resetOffset(RemotingCommand* request);
  RemotingCommand* getConsumerRunningInfo(const std::string& addr, RemotingCommand* request);
  RemotingCommand* notifyConsumerIdsChanged(RemotingCommand* request);
  RemotingCommand* checkTransactionState(const std::string& addr, RemotingCommand* request);

 private:
  MQClientInstance* m_mqClientFactory;
};

class ResetOffsetBody {
 public:
  static ResetOffsetBody* Decode(MemoryBlock& mem);

  std::map<MQMessageQueue, int64_t> getOffsetTable();
  void setOffsetTable(MQMessageQueue mq, int64_t offset);

 private:
  std::map<MQMessageQueue, int64_t> m_offsetTable;
};

class CheckTransactionStateBody {
 public:
  static CheckTransactionStateBody* Decode(MemoryBlock& mem);

  std::map<MQMessageQueue, int64_t> getOffsetTable();
  void setOffsetTable(MQMessageQueue mq, int64_t offset);

 private:
  std::map<MQMessageQueue, int64_t> m_offsetTable;
};

}  // namespace rocketmq

#endif  // __CLIENT_REMOTING_PROCESSOR_H__
