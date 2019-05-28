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
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "ClientRPCHook.h"
#include "CommandHeader.h"
#include "RemotingCommand.h"
#include "SessionCredentials.h"

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::RemotingCommand;
using rocketmq::SendMessageRequestHeader;
using rocketmq::SessionCredentials;

using rocketmq::ClientRPCHook;

TEST(clientRPCHook, doBeforeRequest) {
  SessionCredentials sessionCredentials;
  sessionCredentials.setAccessKey("accessKey");
  sessionCredentials.setSecretKey("secretKey");
  sessionCredentials.setAuthChannel("onsChannel");

  ClientRPCHook clientRPCHook(sessionCredentials);

  RemotingCommand remotingCommand;
  clientRPCHook.doBeforeRequest("127.0.0.1:9876", remotingCommand);

  SendMessageRequestHeader* sendMessageRequestHeader = new SendMessageRequestHeader();
  RemotingCommand headeRremotingCommand(17, sendMessageRequestHeader);
  clientRPCHook.doBeforeRequest("127.0.0.1:9876", headeRremotingCommand);

  headeRremotingCommand.setMsgBody("1231231");
  clientRPCHook.doBeforeRequest("127.0.0.1:9876", headeRremotingCommand);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "clientRPCHook.doBeforeRequest";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
