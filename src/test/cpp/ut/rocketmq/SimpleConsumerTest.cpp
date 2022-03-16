
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
#include <system_error>

#include "ClientManager.h"
#include "ClientManagerFactory.h"
#include "ClientManagerMock.h"
#include "MQClientTest.h"
#include "SimpleConsumerImpl.h"
#include "StaticNameServerResolver.h"
#include "src/test/cpp/ut/rocketmq/_virtual_includes/client_interface/MQClientTest.h"

ROCKETMQ_NAMESPACE_BEGIN

class SimpleConsumerTest : public MQClientTest {};

TEST_F(SimpleConsumerTest, DISABLED_testLifecycle) {
  auto consumer = std::make_shared<SimpleConsumerImpl>(group_name_);
}

ROCKETMQ_NAMESPACE_END