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
#include "ClientRPCHook.h"

#include <string>

#include "DataBlock.h"
#include "Logging.h"
#include "RemotingCommand.h"
#include "protocol/header/CommandHeader.h"
#include "spas_client.h"

namespace rocketmq {

const std::string SessionCredentials::AccessKey = "AccessKey";
const std::string SessionCredentials::SecretKey = "SecretKey";
const std::string SessionCredentials::Signature = "Signature";
const std::string SessionCredentials::SignatureMethod = "SignatureMethod";
const std::string SessionCredentials::ONSChannelKey = "OnsChannel";

void ClientRPCHook::doBeforeRequest(const std::string& remoteAddr, RemotingCommand& request, bool toSent) {
  if (toSent) {
    // sign request
    signCommand(request);
  }
}

void ClientRPCHook::doAfterResponse(const std::string& remoteAddr,
                                    RemotingCommand& request,
                                    RemotingCommand* response,
                                    bool toSent) {
  if (toSent && response != nullptr) {
    // sign response
    signCommand(*response);
  }
}

void ClientRPCHook::signCommand(RemotingCommand& command) {
  std::map<std::string, std::string> headerMap;
  headerMap.insert(std::make_pair(SessionCredentials::AccessKey, sessionCredentials_.getAccessKey()));
  headerMap.insert(std::make_pair(SessionCredentials::ONSChannelKey, sessionCredentials_.getAuthChannel()));

  LOG_DEBUG_NEW("before insert declared filed, MAP SIZE is:{}", headerMap.size());
  auto* header = command.readCustomHeader();
  if (header != nullptr) {
    header->SetDeclaredFieldOfCommandHeader(headerMap);
  }
  LOG_DEBUG_NEW("after insert declared filed, MAP SIZE is:{}", headerMap.size());

  std::string totalMsg;
  for (const auto& it : headerMap) {
    totalMsg.append(it.second);
  }
  auto body = command.getBody();
  if (body != nullptr && body->getSize() > 0) {
    LOG_DEBUG_NEW("request have msgBody, length is:{}", body->getSize());
    totalMsg.append(body->getData(), body->getSize());
  }
  LOG_DEBUG_NEW("total msg info are:{}, size is:{}", totalMsg, totalMsg.size());

  char* sign =
      rocketmqSignature::spas_sign(totalMsg.c_str(), totalMsg.size(), sessionCredentials_.getSecretKey().c_str());
  if (sign != nullptr) {
    std::string signature(static_cast<const char*>(sign));
    command.addExtField(SessionCredentials::Signature, signature);
    command.addExtField(SessionCredentials::AccessKey, sessionCredentials_.getAccessKey());
    command.addExtField(SessionCredentials::ONSChannelKey, sessionCredentials_.getAuthChannel());
    rocketmqSignature::spas_mem_free(sign);
  } else {
    LOG_ERROR_NEW("signature for request failed");
  }
}

}  // namespace rocketmq
