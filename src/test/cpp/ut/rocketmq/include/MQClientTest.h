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
#pragma once

#include <functional>
#include <memory>

#include "google/rpc/code.pb.h"
#include "grpc/grpc.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "ClientManagerFactory.h"
#include "ClientManagerImpl.h"
#include "RpcClientMock.h"
#include "StaticNameServerResolver.h"

ROCKETMQ_NAMESPACE_BEGIN

class MQClientTest : public testing::Test {
public:
  MQClientTest() = default;

  void SetUp() override {
    grpc_init();
    name_server_list_.emplace_back(name_server_address_);
    client_instance_ = std::make_shared<ClientManagerImpl>(resource_namespace_);
    rpc_client_ns_ = std::make_shared<testing::NiceMock<RpcClientMock>>();
    ON_CALL(*rpc_client_ns_, needHeartbeat()).WillByDefault(testing::Return(false));
    ON_CALL(*rpc_client_ns_, ok()).WillByDefault(testing::Invoke([this]() { return client_ok_; }));
    ON_CALL(*rpc_client_ns_, asyncQueryRoute(testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            std::bind(&MQClientTest::mockQueryRoute, this, std::placeholders::_1, std::placeholders::_2)));
    name_server_resolver_ = std::make_shared<StaticNameServerResolver>(name_server_address_);
    client_instance_->addRpcClient(name_server_resolver_->resolve(), rpc_client_ns_);
    ClientManagerFactory::getInstance().addClientManager(resource_namespace_, client_instance_);
  }

  void TearDown() override {
    rpc_client_ns_.reset();
    client_instance_->cleanRpcClients();
    client_instance_.reset();
    grpc_shutdown();
  }

protected:
  std::shared_ptr<ClientManagerImpl> client_instance_;
  std::shared_ptr<testing::NiceMock<RpcClientMock>> rpc_client_ns_;
  const int16_t port_{10911};
  const int32_t partition_num_{24};
  const int32_t avg_partition_per_host_{8};
  std::string name_server_address_{"127.0.0.1:9876"};
  std::shared_ptr<NameServerResolver> name_server_resolver_;
  std::string group_name_{"CID_Test"};
  std::string topic_{"Topic_Test"};
  std::string resource_namespace_{"mq://test"};
  google::rpc::Code ok_{google::rpc::Code::OK};
  bool client_ok_{true};
  std::vector<std::string> name_server_list_;

  void mockQueryRoute(const QueryRouteRequest& request,
                      InvocationContext<QueryRouteResponse>* invocation_context) const {
    invocation_context->response.mutable_common()->mutable_status()->set_code(google::rpc::Code::OK);
    auto partitions = invocation_context->response.mutable_partitions();
    for (int i = 0; i < partition_num_; i++) {
      auto partition = new rmq::Partition();
      partition->set_id(i % avg_partition_per_host_);
      partition->set_permission(rmq::Permission::READ_WRITE);
      partition->mutable_topic()->set_name(request.topic().name());
      partition->mutable_topic()->set_resource_namespace(request.topic().resource_namespace());
      std::string broker_name{"broker-"};
      broker_name.push_back('a' + i / avg_partition_per_host_);
      partition->mutable_broker()->set_name(broker_name);

      auto endpoint = partition->mutable_broker()->mutable_endpoints();
      endpoint->set_scheme(rmq::AddressScheme::IPv4);
      auto addresses = endpoint->mutable_addresses();
      auto address = new rmq::Address;
      address->set_host(fmt::format("10.0.0.{}", i / avg_partition_per_host_));
      address->set_port(port_);
      addresses->AddAllocated(address);
      partitions->AddAllocated(partition);
    }
    // Mock invoke callback.
    invocation_context->onCompletion(true);
  }
};

ROCKETMQ_NAMESPACE_END