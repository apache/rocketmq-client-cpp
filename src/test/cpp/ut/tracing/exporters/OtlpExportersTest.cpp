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
  void SetUp() override { client_manager_ = std::make_shared<testing::NiceMock<ClientManagerMock>>(); }

  void TearDown() override {}

protected:
  std::shared_ptr<testing::NiceMock<ClientManagerMock>> client_manager_;
  ClientConfigMock client_config_;
};

TEST_F(OtlpExporterTest, testExport) {
  auto exporter = std::make_shared<OtlpExporter>(client_manager_, &client_config_);
  exporter->traceMode(TraceMode::DEBUG);
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