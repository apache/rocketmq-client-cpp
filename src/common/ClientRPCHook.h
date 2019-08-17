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
#ifndef __CLIENT_RPC_HOOK_H__
#define __CLIENT_RPC_HOOK_H__

#include "RemotingCommand.h"
#include "SessionCredentials.h"

namespace rocketmq {

class RPCHook {
 public:
  RPCHook() = default;
  virtual ~RPCHook() = default;

  virtual void doBeforeRequest(const std::string& remoteAddr, RemotingCommand& request) = 0;
  virtual void doAfterResponse(RemotingCommand& request, RemotingCommand& response) = 0;
};

class ClientRPCHook : public RPCHook {
 private:
  SessionCredentials sessionCredentials;

 public:
  ClientRPCHook(const SessionCredentials& session_credentials) : sessionCredentials(session_credentials) {}
  ~ClientRPCHook() override = default;

  void doBeforeRequest(const std::string& remoteAddr, RemotingCommand& request) override;

  void doAfterResponse(RemotingCommand& request, RemotingCommand& response) override {}
};

}  // namespace rocketmq

#endif  // __CLIENT_RPC_HOOK_H__
