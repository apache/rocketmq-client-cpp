#pragma once

#include <memory>
#include <string>

#include "AsyncCallback.h"
#include "ConsumerType.h"
#include "ExpressionType.h"
#include "Logger.h"
#include "MQMessageListener.h"
#include "MQMessageQueue.h"
#include "CredentialsProvider.h"

ROCKETMQ_NAMESPACE_BEGIN

class DefaultMQPushConsumerImpl;

class DefaultMQPushConsumer {
public:
  explicit DefaultMQPushConsumer(const std::string& group_name);

  ~DefaultMQPushConsumer() = default;

  void start();

  void shutdown();

  void subscribe(const std::string& topic, const std::string& expression,
                 ExpressionType expression_type = ExpressionType::TAG);

  void setConsumeFromWhere(ConsumeFromWhere policy);

  void registerMessageListener(MQMessageListener* listener);

  void setNamesrvAddr(const std::string& name_srv);

  void setGroupName(const std::string& group_name);

  void setConsumeThreadCount(int thread_count);

  void setInstanceName(const std::string& instance_name);

  int getProcessQueueTableSize();

  void setUnitName(std::string unit_name);

  const std::string& getUnitName() const;

  void enableTracing(bool enabled);

  bool isTracingEnabled();

  /**
   * SDK of this version always uses asynchronous IO operation. As such, this function is no-op
   * to keep backward compatibility.
   */
  void setAsyncPull(bool);

  /**
   * By default maxCacheMsgSize is 1024. Valid range is: 1~65535. Should not change this value unless you know what
   * you are doing.
   */
  void setMaxCacheMsgSizePerQueue(int max_cache_size);

  /**
   * Maximum number of messages passed to each callback.
   * @param batch_size Batch size
   */
  void setConsumeMessageBatchMaxSize(int batch_size);

  /**
   * Lifecycle of executor is managed by external application. Passed-in executor should remain valid after consumer
   * start and before stopping.
   * @param executor Executor pool used to invoke consume callback.
   */
  void setCustomExecutor(const Executor& executor);

  /**
   * This function sets maximum number of message that may be consumed per second.
   * @param topic Topic to control
   * @param threshold Threshold before throttling is enforced.
   */
  void setThrottle(const std::string& topic, uint32_t threshold);

  /**
   * Set abstract-resource-namespace, in which canonical name of topic, group remains unique.
   * @param arn Abstract resource namespace.
   */
  void setArn(const char *arn);

  void setCredentialsProvider(CredentialsProviderPtr credentials_provider);

  void setMessageModel(MessageModel message_model);

private:
  std::shared_ptr<DefaultMQPushConsumerImpl> impl_;
  std::string group_name_;
};

ROCKETMQ_NAMESPACE_END