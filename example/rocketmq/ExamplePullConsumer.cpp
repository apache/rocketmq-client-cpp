#include "rocketmq/CredentialsProvider.h"
#include "rocketmq/DefaultMQPullConsumer.h"
#include "rocketmq/Logger.h"
#include <cstdlib>

int main(int argc, char* argv[]) {
  const char* group = "GID_group003";
  const char* topic = "yc001";
  const char* resource_namespace = "MQ_INST_1973281269661160_BXmPlOA6";
  const char* name_server_list = "ipv4:11.165.223.199:9876";

  rocketmq::Logger& logger = rocketmq::getLogger();
  logger.setLevel(rocketmq::Level::Debug);
  logger.init();

  rocketmq::DefaultMQPullConsumer pull_consumer(group);
  pull_consumer.setResourceNamespace(resource_namespace);
  pull_consumer.setCredentialsProvider(std::make_shared<rocketmq::ConfigFileCredentialsProvider>());
  pull_consumer.setNamesrvAddr(name_server_list);
  pull_consumer.start();

  std::future<std::vector<rocketmq::MQMessageQueue>> future = pull_consumer.queuesFor(topic);
  auto queues = future.get();

  for (const auto& queue : queues) {
    rocketmq::OffsetQuery offset_query;
    offset_query.message_queue = queue;
    offset_query.policy = rocketmq::QueryOffsetPolicy::BEGINNING;
    auto offset_future = pull_consumer.queryOffset(offset_query);
    int64_t offset = offset_future.get();
  }

  pull_consumer.shutdown();
  return EXIT_SUCCESS;
}