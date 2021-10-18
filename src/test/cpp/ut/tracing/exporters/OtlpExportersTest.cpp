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
#include "ClientConfigMock.h"
#include "ClientManagerMock.h"
#include "OtlpExporter.h"
#include "opencensus/trace/sampler.h"
#include "opencensus/trace/span.h"
#include "rocketmq/RocketMQ.h"
#include "gtest/gtest.h"
#include <chrono>
#include <memory>
#include <thread>

ROCKETMQ_NAMESPACE_BEGIN

class OtlpExporterTest : public testing::Test {
public:
  void SetUp() override {
    client_manager_ = std::make_shared<testing::NiceMock<ClientManagerMock>>();
  }

  void TearDown() override {
  }

protected:
  std::shared_ptr<testing::NiceMock<ClientManagerMock>> client_manager_;
  ClientConfigMock client_config_;
};

TEST_F(OtlpExporterTest, testExport) {
  auto exporter = std::make_shared<OtlpExporter>(client_manager_, &client_config_);
  exporter->traceMode(TraceMode::Develop);
  exporter->start();

  auto& sampler = Samplers::always();

  auto span_generator = [&] {
    int total = 20;
    while (total) {
      auto span = opencensus::trace::Span::StartSpan("TestSpan", nullptr, {&sampler});
      span.AddAnnotation("annotation-1", {{"key-1", "value-1"}, {"key-2", 2}, {"key-3", true}});
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      span.End();
      if (0 == --total % 10) {
        std::cout << "Total: " << total << std::endl;
      }
    }
  };

  std::thread t(span_generator);
  if (t.joinable()) {
    t.join();
  }
}

ROCKETMQ_NAMESPACE_END