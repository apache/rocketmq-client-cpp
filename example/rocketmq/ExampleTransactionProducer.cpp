#include "rocketmq/DefaultMQProducer.h"
#include <cstdlib>

using namespace ROCKETMQ_NAMESPACE;

int main(int argc, char* argv[]) {
  DefaultMQProducer producer("TestGroup");

  const char* topic = "cpp_sdk_standard";
  const char* name_server = "47.98.116.189:80";

  producer.setNamesrvAddr(name_server);
  producer.compressBodyThreshold(256);
  const char* resource_namespace = "MQ_INST_1080056302921134_BXuIbML7";
  producer.setRegion("cn-hangzhou-pre");
  producer.setResourceNamespace(resource_namespace);
  producer.setCredentialsProvider(std::make_shared<ConfigFileCredentialsProvider>());

  MQMessage message;
  message.setTopic(topic);
  message.setTags("TagA");
  message.setKey("Yuck! Why-plural?");
  message.setBody("ABC");

  producer.start();

  auto transaction = producer.prepare(message);

  transaction->commit();

  producer.shutdown();

  return EXIT_SUCCESS;
}