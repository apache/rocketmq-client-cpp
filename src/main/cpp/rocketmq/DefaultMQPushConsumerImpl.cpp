#include "DefaultMQPushConsumerImpl.h"
#include "ClientManager.h"

#include "MessageAccessor.h"
#include "MixAll.h"
#include "Signature.h"
#include "rocketmq/MQClientException.h"
#include <chrono>

ROCKETMQ_NAMESPACE_BEGIN

AsyncReceiveMessageCallback::AsyncReceiveMessageCallback(ProcessQueueWeakPtr process_queue)
    : process_queue_(std::move(process_queue)) {
  receive_message_later_ = std::bind(&AsyncReceiveMessageCallback::checkThrottleThenReceive, this);
  receive_message_later_functional_ = std::make_shared<Functional>(&receive_message_later_);
}

void AsyncReceiveMessageCallback::onSuccess(ReceiveMessageResult& result) {
  ProcessQueueSharedPtr process_queue_shared_ptr = process_queue_.lock();
  if (!process_queue_shared_ptr) {
    SPDLOG_WARN("Process queue has been released. Drop PopResult: {}", result.toString());
    return;
  }

  int64_t term_id = 0;
  if (!result.termId(term_id)) {
    SPDLOG_WARN("Failed to decode Term-Id from Request-Id: {}", result.requestId());
  }

  std::shared_ptr<DefaultMQPushConsumerImpl> impl = process_queue_shared_ptr->getCallbackOwner().lock();
  if (impl->isStopped()) {
    return;
  }

  switch (result.status()) {
  case ReceiveMessageStatus::OK:
    SPDLOG_DEBUG("Receive messages from broker[host={}] returns with status=FOUND, msgListSize={}, queue={}",
                 result.sourceHost(), result.getMsgFoundList().size(), process_queue_shared_ptr->simpleName());
    process_queue_shared_ptr->cacheMessages(result.getMsgFoundList());
    impl->getConsumeMessageService()->dispatch();

    if (process_queue_shared_ptr->consumeType() == ConsumeMessageType::PULL) {
      process_queue_shared_ptr->nextOffset(result.next_offset_);
    }

    if (process_queue_shared_ptr->leaseNextTerm(term_id, result.sourceHost())) {
      checkThrottleThenReceive();
    }
    break;
  case ReceiveMessageStatus::DATA_CORRUPTED:
    if (process_queue_shared_ptr->consumeType() == POP) {
      process_queue_shared_ptr->cacheMessages(result.messages_);
      impl->getConsumeMessageService()->dispatch();
    }
    if (process_queue_shared_ptr->leaseNextTerm(term_id, result.sourceHost())) {
      checkThrottleThenReceive();
    }
    break;

  case ReceiveMessageStatus::OUT_OF_RANGE:
    assert(ConsumeMessageType::PULL == process_queue_shared_ptr->consumeType());
    process_queue_shared_ptr->nextOffset(result.next_offset_);
    if (process_queue_shared_ptr->leaseNextTerm(term_id, result.sourceHost())) {
      checkThrottleThenReceive();
    }
    break;
  case ReceiveMessageStatus::DEADLINE_EXCEEDED:
    SPDLOG_DEBUG("Receive messages from broker[host={}] returns with status=DEADLINE_EXCEEDED, queue={}",
                 result.sourceHost(), process_queue_shared_ptr->simpleName());
    if (process_queue_shared_ptr->leaseNextTerm(term_id, result.sourceHost())) {
      checkThrottleThenReceive();
    }
    break;
  case ReceiveMessageStatus::INTERNAL:
    SPDLOG_DEBUG("Receive messages from broker[host={}] returns with status=UNKNOWN, queue={}", result.sourceHost(),
                 process_queue_shared_ptr->simpleName());
    if (process_queue_shared_ptr->leaseNextTerm(term_id, result.sourceHost())) {
      receiveMessageLater();
    }
    break;
  case ReceiveMessageStatus::RESOURCE_EXHAUSTED:
    SPDLOG_DEBUG("Receive messages from broker[host={}] returns with status=RESOURCE_EXHAUSTED, queue={}",
                 result.sourceHost(), process_queue_shared_ptr->simpleName());
    if (process_queue_shared_ptr->leaseNextTerm(term_id, result.sourceHost())) {
      receiveMessageLater();
    }
    break;
  case ReceiveMessageStatus::NOT_FOUND:
    SPDLOG_DEBUG("Receive messages from broker[host={}] returns with status=NOT_FOUND, queue={}", result.sourceHost(),
                 process_queue_shared_ptr->simpleName());
    if (process_queue_shared_ptr->leaseNextTerm(term_id, result.sourceHost())) {
      receiveMessageLater();
    }
    break;
  default:
    SPDLOG_WARN("Unknown receive message status: {} from broker[host={}], queue={}", result.status(),
                result.sourceHost(), process_queue_shared_ptr->simpleName());
    receiveMessageLater();
    break;
  }
}

void AsyncReceiveMessageCallback::checkThrottleThenReceive() {
  auto process_queue = process_queue_.lock();
  if (!process_queue) {
    SPDLOG_WARN("Process queue should have been destructed");
    return;
  }

  if (process_queue->shouldThrottle()) {
    SPDLOG_INFO("Number of messages in {} exceeds throttle threshold. Receive messages later.",
                process_queue->simpleName());
    process_queue->updateThrottleTimestamp();
    receiveMessageLater();
  } else {
    // Receive message immediately
    receiveMessageImmediately();
  }
}

void AsyncReceiveMessageCallback::onException(MQException& e) {
  auto process_queue_ptr = process_queue_.lock();
  if (process_queue_ptr) {
    SPDLOG_WARN("pop message error:{}, pop message later. Queue={}", e.what(), process_queue_ptr->simpleName());
    // pop message later
    receiveMessageLater();
  }
}

void AsyncReceiveMessageCallback::receiveMessageLater() {
  auto process_queue = process_queue_.lock();
  if (!process_queue) {
    return;
  }

  auto client_instance = process_queue->getClientInstance();
  bool scheduled = client_instance->getScheduler().schedule(receive_message_later_functional_, std::chrono::seconds(1),
                                                            std::chrono::seconds(0));
  if (scheduled) {
    SPDLOG_INFO("Receive message later scheduled");
  } else {
    SPDLOG_ERROR("Failed to schedule receive message later");
  }
}

void AsyncReceiveMessageCallback::receiveMessageImmediately() {
  ProcessQueueSharedPtr process_queue_shared_ptr = process_queue_.lock();
  if (!process_queue_shared_ptr) {
    SPDLOG_INFO("ProcessQueue has been released. Ignore further receive message request-response cycles");
    return;
  }

  std::shared_ptr<DefaultMQPushConsumerImpl> impl = process_queue_shared_ptr->getCallbackOwner().lock();
  if (!impl) {
    SPDLOG_INFO("Owner of ProcessQueue[{}] has been released. Ignore further receive message request-response cycles",
                process_queue_shared_ptr->simpleName());
    return;
  }
  impl->receiveMessage(process_queue_shared_ptr->getMQMessageQueue(), process_queue_shared_ptr->getFilterExpression(),
                       process_queue_shared_ptr->consumeType());
}

const int32_t DefaultMQPushConsumerImpl::MAX_CACHED_MESSAGE_COUNT = 65535;
const int32_t DefaultMQPushConsumerImpl::DEFAULT_CACHED_MESSAGE_COUNT = 1024;
const int32_t DefaultMQPushConsumerImpl::DEFAULT_CONSUME_MESSAGE_BATCH_SIZE = 1;
const int32_t DefaultMQPushConsumerImpl::DEFAULT_CONSUME_THREAD_POOL_SIZE = 20;

DefaultMQPushConsumerImpl::DefaultMQPushConsumerImpl(std::string group_name)
    : BaseImpl(std::move(group_name)), consume_thread_pool_size_(DEFAULT_CONSUME_THREAD_POOL_SIZE),
      message_listener_ptr_(nullptr), consume_batch_size_(DEFAULT_CONSUME_MESSAGE_BATCH_SIZE),
      max_cached_message_number_per_queue_(DEFAULT_CACHED_MESSAGE_COUNT) {
  scan_assignment_executor_ = std::bind(&DefaultMQPushConsumerImpl::scanAssignments, this);
  scan_assignment_functional_ = std::make_shared<Functional>(&scan_assignment_executor_);
}

DefaultMQPushConsumerImpl::~DefaultMQPushConsumerImpl() { SPDLOG_DEBUG("DefaultMQPushConsumerImpl is destructed"); }

void DefaultMQPushConsumerImpl::start() {
  BaseImpl::start();

  if (State::STARTED != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected consumer state: {}", state_.load(std::memory_order_relaxed));
    return;
  }

  client_instance_->addClientObserver(shared_from_this());

  fetchRoutes();
  heartbeat();

  if (message_listener_ptr_) {
    if (message_listener_ptr_->getMessageListenerType() == messageListenerOrderly) {
      SPDLOG_INFO("start orderly consume service: {}", group_name_);
      consume_message_service_ = std::make_shared<ConsumeMessageOrderlyService>(
          shared_from_this(), consume_thread_pool_size_, message_listener_ptr_);
    } else {
      // For backward compatibility, by default, ConsumeMessageConcurrentlyService is assumed.
      SPDLOG_INFO("start concurrently consume service: {}", group_name_);
      consume_message_service_ = std::make_shared<ConsumeMessageConcurrentlyService>(
          shared_from_this(), consume_thread_pool_size_, message_listener_ptr_);
    }
    consume_message_service_->start();

    {
      // Set consumer throttling
      absl::MutexLock lock(&throttle_table_mtx_);
      for (const auto& item : throttle_table_) {
        consume_message_service_->throttle(item.first, item.second);
      }
    }
  }

  bool scheduled;
  scheduled = client_instance_->getScheduler().schedule(scan_assignment_functional_, std::chrono::seconds(0),
                                                        std::chrono::seconds(5));
  if (scheduled) {
    SPDLOG_INFO("pop message executor scheduled");
  } else {
    SPDLOG_ERROR("Failed to schedule pop message executor");
  }
  state_.store(State::STARTED, std::memory_order_relaxed);
  SPDLOG_INFO("DefaultMQPushConsumer started, groupName={}", group_name_);
}

void DefaultMQPushConsumerImpl::shutdown() {
  state_.store(State::STOPPING, std::memory_order_relaxed);
  bool cancelled;
  cancelled = client_instance_->getScheduler().cancel(&scan_assignment_executor_);
  if (cancelled) {
    SPDLOG_INFO("pop message executor cancelled OK");
  } else {
    SPDLOG_ERROR("Failed to cancel pop message executor");
  }

  {
    absl::MutexLock lock(&process_queue_table_mtx_);
    process_queue_table_.clear();
  }

  if (consume_message_service_) {
    consume_message_service_->shutdown();
  }

  // Destruct client instance as soon as possible.
  client_instance_.reset();
}

bool DefaultMQPushConsumerImpl::isStopped() const {
  State current_state = state_.load(std::memory_order_relaxed);
  return State::STOPPED == current_state || State::STOPPING == current_state;
}

void DefaultMQPushConsumerImpl::subscribe(const std::string& topic, const std::string& expression,
                                          ExpressionType expression_type) {
  absl::MutexLock lock(&topic_filter_expression_table_mtx_);
  FilterExpression filter_expression{expression, expression_type};
  topic_filter_expression_table_.emplace(topic, filter_expression);
}

void DefaultMQPushConsumerImpl::unsubscribe(const std::string& topic) {
  absl::MutexLock lock(&topic_filter_expression_table_mtx_);
  topic_filter_expression_table_.erase(topic);
}

absl::flat_hash_map<std::string, FilterExpression> DefaultMQPushConsumerImpl::getTopicFilterExpressionTable() {
  absl::flat_hash_map<std::string, FilterExpression> topic_filter_expression_table;
  {
    absl::MutexLock lock(&topic_filter_expression_table_mtx_);
    for (auto item : topic_filter_expression_table_) {
      topic_filter_expression_table.emplace(item.first, item.second);
    }
  }

  return topic_filter_expression_table;
}

void DefaultMQPushConsumerImpl::setConsumeFromWhere(ConsumeFromWhere consume_from_where) {
  consume_from_where_ = consume_from_where;
}

void DefaultMQPushConsumerImpl::scanAssignments() {
  SPDLOG_DEBUG("Start of assignment scanning");
  if (isStopped()) {
    SPDLOG_INFO("Client has stopped. Abort scanning immediately.");
    return;
  }
  absl::flat_hash_map<std::string, FilterExpression> topic_filter_expression_table = getTopicFilterExpressionTable();
  {
    for (auto& filter_entry : topic_filter_expression_table) {
      std::string topic = filter_entry.first;
      FilterExpression filter_expression = filter_entry.second;
      SPDLOG_DEBUG("Scan assignments for {}", topic);
      auto callback = [this, topic, filter_expression](const TopicAssignmentPtr& assignments) {
        if (assignments && !assignments->assignmentList().empty()) {
          syncProcessQueue(topic, assignments, filter_expression);
        } else {
          SPDLOG_WARN("Failed to acquire assignments for topic={} from load balancer for the first time", topic);
        }
      };
      queryAssignment(topic, callback);
    } // end of for-loop
  }
  SPDLOG_DEBUG("End of assignment scanning.");
}

bool DefaultMQPushConsumerImpl::selectBroker(const TopicRouteDataPtr& topic_route_data, std::string& broker_host) {
  if (topic_route_data && !topic_route_data->partitions().empty()) {
    uint32_t index = TopicAssignment::getAndIncreaseQueryWhichBroker();
    for (uint32_t i = index; i < index + topic_route_data->partitions().size(); i++) {
      auto partition = topic_route_data->partitions().at(i % topic_route_data->partitions().size());
      if (MixAll::MASTER_BROKER_ID != partition.broker().id() || Permission::NONE == partition.permission()) {
        continue;
      }

      if (partition.broker()) {
        broker_host = partition.broker().serviceAddress();
        return true;
      }
    }
  }
  return false;
}

void DefaultMQPushConsumerImpl::wrapQueryAssignmentRequest(const std::string& topic, const std::string& consumer_group,
                                                           const std::string& client_id,
                                                           const std::string& strategy_name,
                                                           QueryAssignmentRequest& request) {
  request.mutable_topic()->set_name(topic);
  request.mutable_topic()->set_arn(arn());
  request.mutable_group()->set_name(consumer_group);
  request.mutable_group()->set_arn(arn());
  request.set_client_id(client_id);
}

void DefaultMQPushConsumerImpl::queryAssignment(const std::string& topic,
                                                const std::function<void(const TopicAssignmentPtr&)>& cb) {

  auto callback = [this, topic, cb](const TopicRouteDataPtr& topic_route) {
    TopicAssignmentPtr topic_assignment;
    if (MessageModel::BROADCASTING == message_model_) {
      if (!topic_route) {
        SPDLOG_WARN("Failed to get valid route entries for topic={}", topic);
        cb(topic_assignment);
      }

      std::vector<Assignment> assignments;
      assignments.reserve(topic_route->partitions().size());
      for (const auto& partition : topic_route->partitions()) {
        assignments.emplace_back(Assignment(partition.asMessageQueue(), ConsumeMessageType::PULL));
      }
      topic_assignment = std::make_shared<TopicAssignment>(std::move(assignments));
      cb(topic_assignment);
      return;
    }

    std::string broker_host;
    if (!selectBroker(topic_route, broker_host)) {
      SPDLOG_WARN("Failed to select a broker to query assignment for group={}, topic={}", group_name_, topic);
    }

    QueryAssignmentRequest request;
    wrapQueryAssignmentRequest(topic, group_name_, clientId(), MixAll::DEFAULT_LOAD_BALANCER_STRATEGY_NAME_, request);

    absl::flat_hash_map<std::string, std::string> metadata;
    Signature::sign(this, metadata);

    auto assignment_callback = [this, cb, topic, broker_host](bool ok, const QueryAssignmentResponse& response) {
      if (ok) {
        SPDLOG_DEBUG("Query topic assignment OK. Topic={}, group={}, assignment-size={}", topic, group_name_,
                     response.assignments().size());
        SPDLOG_TRACE("Query assignment response for {} is: {}", topic, ptr->debugString());
        cb(std::make_shared<TopicAssignment>(response));
      } else {
        SPDLOG_WARN("Failed to acquire queue assignment of topic={} from brokerAddress={}", topic, broker_host);
        cb(nullptr);
      }
    };

    client_instance_->queryAssignment(broker_host, metadata, request, absl::ToChronoMilliseconds(io_timeout_),
                                      assignment_callback);
  };
  getRouteFor(topic, callback);
}

/**
 *
 * @param topic Topic to process
 * @param assignment Latest assignment from load balancer
 * @param filter_expression Filter expression
 */
void DefaultMQPushConsumerImpl::syncProcessQueue(const std::string& topic,
                                                 const std::shared_ptr<TopicAssignment>& topic_assignment,
                                                 const FilterExpression& filter_expression) {
  const std::vector<Assignment>& assignment_list = topic_assignment->assignmentList();
  std::vector<MQMessageQueue> message_queue_list;
  message_queue_list.reserve(assignment_list.size());
  for (const auto& assignment : assignment_list) {
    message_queue_list.push_back(assignment.messageQueue());
  }

  std::vector<MQMessageQueue> current;
  {
    absl::MutexLock lock(&process_queue_table_mtx_);
    for (auto it = process_queue_table_.begin(); it != process_queue_table_.end();) {
      if (topic != it->first.getTopic()) {
        it++;
        continue;
      }

      if (std::none_of(message_queue_list.cbegin(), message_queue_list.cend(),
                       [&](const MQMessageQueue& message_queue) { return it->first == message_queue; })) {
        SPDLOG_INFO("Stop receiving messages from {} as it is not assigned to current client according to latest "
                    "assignment result from load balancer",
                    it->first.simpleName());
        process_queue_table_.erase(it++);
      } else {
        if (!it->second || it->second->expired()) {
          SPDLOG_WARN("ProcessQueue={} is expired. Remove it for now.", it->first.simpleName());
          process_queue_table_.erase(it++);
          continue;
        }
        current.push_back(it->first);
        it++;
      }
    }
  }

  for (const auto& message_queue : message_queue_list) {
    if (std::none_of(current.cbegin(), current.cend(),
                     [&](const MQMessageQueue& item) { return item == message_queue; })) {
      SPDLOG_INFO("Start to receive message from {} according to latest assignment info from load balancer",
                  message_queue.simpleName());
      // Clustering message model is implemented through POP while broadcasting is implemented through classic PULL.
      ConsumeMessageType consume_type =
          MessageModel::CLUSTERING == message_model_ ? ConsumeMessageType::POP : ConsumeMessageType::PULL;
      if (!receiveMessage(message_queue, filter_expression, consume_type)) {
        if (!isStopped()) {
          SPDLOG_WARN("Failed to initiate receive message request-response-cycle for {}", message_queue.simpleName());
          // TODO: remove it from current assignment such that a second attempt will be made again in the next round.
        }
      }
    }
  }
}

ProcessQueueSharedPtr DefaultMQPushConsumerImpl::getOrCreateProcessQueue(const MQMessageQueue& message_queue,
                                                                         const FilterExpression& filter_expression,
                                                                         ConsumeMessageType consume_type) {
  ProcessQueueSharedPtr process_queue;
  {
    absl::MutexLock lock(&process_queue_table_mtx_);
    if (isStopped()) {
      SPDLOG_INFO("DefaultMQPushConsumer has stopped. Drop creation of ProcessQueue");
      return process_queue;
    }

    if (process_queue_table_.contains(message_queue)) {
      process_queue = process_queue_table_.at(message_queue);
    } else {
      SPDLOG_INFO("Create ProcessQueue for message queue[{}]", message_queue.simpleName());
      // create ProcessQueue
      process_queue =
          std::make_shared<ProcessQueue>(message_queue, filter_expression, consume_type,
                                         max_cached_message_number_per_queue_, shared_from_this(), client_instance_);
      std::shared_ptr<AsyncReceiveMessageCallback> receive_callback =
          std::make_shared<AsyncReceiveMessageCallback>(process_queue);
      process_queue->callback(receive_callback);
      process_queue_table_.emplace(std::make_pair(message_queue, process_queue));
    }
  }
  return process_queue;
}

bool DefaultMQPushConsumerImpl::receiveMessage(const MQMessageQueue& message_queue,
                                               const FilterExpression& filter_expression,
                                               ConsumeMessageType consume_type) {
  if (isStopped()) {
    SPDLOG_INFO("DefaultMQPushConsumer has stopped. Drop further receive message request");
    return false;
  }

  ProcessQueueSharedPtr process_queue_ptr = getOrCreateProcessQueue(message_queue, filter_expression, consume_type);
  if (!process_queue_ptr) {
    SPDLOG_INFO("consumer has stopped. Stop creating processQueue");
    return false;
  }

  const std::string& broker_host = message_queue.serviceAddress();
  if (broker_host.empty()) {
    SPDLOG_ERROR("Failed to resolve address for brokerName={}", message_queue.getBrokerName());
    return false;
  }

  switch (consume_type) {
  case ConsumeMessageType::PULL: {
    int64_t offset = -1;
    if (!offset_store_ || !offset_store_->readOffset(message_queue, offset)) {
      // Query latest offset from server.
      QueryOffsetRequest request;
      request.mutable_partition()->mutable_topic()->set_arn(arn_);
      request.mutable_partition()->mutable_topic()->set_name(message_queue.getTopic());
      request.mutable_partition()->set_id(message_queue.getQueueId());
      request.mutable_partition()->mutable_broker()->set_name(message_queue.getBrokerName());
      request.set_policy(rmq::QueryOffsetPolicy::END);
      absl::flat_hash_map<std::string, std::string> metadata;
      Signature::sign(this, metadata);
      auto callback = [broker_host, message_queue, process_queue_ptr](bool ok, const QueryOffsetResponse& response) {
        if (ok) {
          assert(response.offset() >= 0);
          process_queue_ptr->nextOffset(response.offset());
          process_queue_ptr->receiveMessage();
        } else {
          SPDLOG_WARN("Failed to acquire latest offset for partition[{}] from server[host={}]",
                      message_queue.simpleName(), broker_host);
        }
      };
      client_instance_->queryOffset(broker_host, metadata, request, absl::ToChronoMilliseconds(io_timeout_), callback);
    }
    break;
  }
  case ConsumeMessageType::POP:
    process_queue_ptr->receiveMessage();
    break;
  }
  return true;
}

std::shared_ptr<ConsumeMessageService> DefaultMQPushConsumerImpl::getConsumeMessageService() {
  return consume_message_service_;
}

void DefaultMQPushConsumerImpl::ack(const MQMessageExt& msg, const std::function<void(bool)>& callback) {
  const std::string& target_host = MessageAccessor::targetEndpoint(msg);
  SPDLOG_DEBUG("Prepare to send ack to broker. BrokerAddress={}, topic={}, queueId={}, msgId={}", target_host,
               msg.getTopic(), msg.getQueueId(), msg.getMsgId());
  AckMessageRequest request;
  wrapAckMessageRequest(msg, request);
  absl::flat_hash_map<std::string, std::string> metadata;
  Signature::sign(this, metadata);
  client_instance_->ack(target_host, metadata, request, absl::ToChronoMilliseconds(io_timeout_), callback);
}

void DefaultMQPushConsumerImpl::nack(const MQMessageExt& msg, const std::function<void(bool)>& callback) {
  std::string target_host = MessageAccessor::targetEndpoint(msg);
  SPDLOG_DEBUG("Send message nack to broker. brokerAddress={}", target_host);

  absl::flat_hash_map<std::string, std::string> metadata;
  Signature::sign(this, metadata);

  rmq::NackMessageRequest request;

  // Group
  request.mutable_group()->set_arn(arn_);
  request.mutable_group()->set_name(group_name_);
  // Topic
  request.mutable_topic()->set_arn(arn_);
  request.mutable_topic()->set_name(msg.getTopic());
  request.set_client_id(clientId());
  request.set_receipt_handle(msg.receiptHandle());
  request.set_message_id(msg.getMsgId());
  request.set_reconsume_times(msg.getReconsumeTimes() + 1);
  request.set_max_reconsume_times(max_delivery_attempts_);

  client_instance_->nack(target_host, metadata, request, absl::ToChronoMilliseconds(io_timeout_), callback);
}

void DefaultMQPushConsumerImpl::wrapAckMessageRequest(const MQMessageExt& msg, AckMessageRequest& request) {
  request.mutable_group()->set_arn(arn_);
  request.mutable_group()->set_name(group_name_);
  request.mutable_topic()->set_arn(arn_);
  request.mutable_topic()->set_name(msg.getTopic());
  request.set_client_id(clientId());
  request.set_message_id(msg.getMsgId());
  request.set_receipt_handle(msg.receiptHandle());
}

int DefaultMQPushConsumerImpl::consumeThreadPoolSize() const { return consume_thread_pool_size_; }

void DefaultMQPushConsumerImpl::consumeThreadPoolSize(int thread_pool_size) {
  if (thread_pool_size >= 1) {
    consume_thread_pool_size_ = thread_pool_size;
  }
}

uint32_t DefaultMQPushConsumerImpl::consumeBatchSize() const { return consume_batch_size_; }

void DefaultMQPushConsumerImpl::consumeBatchSize(uint32_t consume_batch_size) {
  if (consume_batch_size >= 1) {
    consume_batch_size_ = consume_batch_size;
  }
}

void DefaultMQPushConsumerImpl::maxCachedMessageNumberPerQueue(int max_cached_message_number_per_queue) {
  if (max_cached_message_number_per_queue > 0 && max_cached_message_number_per_queue < MAX_CACHED_MESSAGE_COUNT) {
    max_cached_message_number_per_queue_ = max_cached_message_number_per_queue;
  } else {
    max_cached_message_number_per_queue_ = DEFAULT_CACHED_MESSAGE_COUNT;
  }
}

void DefaultMQPushConsumerImpl::registerMessageListener(MQMessageListener* message_listener) {
  message_listener_ptr_ = message_listener;
}

int DefaultMQPushConsumerImpl::getProcessQueueTableSize() {
  absl::MutexLock lock(&process_queue_table_mtx_);
  return process_queue_table_.size();
}

void DefaultMQPushConsumerImpl::setThrottle(const std::string& topic, uint32_t threshold) {
  absl::MutexLock lock(&throttle_table_mtx_);
  throttle_table_.emplace(topic, threshold);
  // If consumer has started, update it dynamically.
  if (getConsumeMessageService()) {
    getConsumeMessageService()->throttle(topic, threshold);
  }
}

#ifdef ENABLE_TRACING
nostd::shared_ptr<trace::Tracer> DefaultMQPushConsumerImpl::getTracer() {
  if (nullptr == client_instance_) {
    return nostd::shared_ptr<trace::Tracer>(nullptr);
  }
  return client_instance_->getTracer();
}
#endif

void DefaultMQPushConsumerImpl::iterateProcessQueue(const std::function<void(ProcessQueueSharedPtr)>& functor) {
  absl::MutexLock lock(&process_queue_table_mtx_);
  for (const auto& item : process_queue_table_) {
    functor(item.second);
  }
}

void DefaultMQPushConsumerImpl::fetchRoutes() {
  std::vector<std::string> topics;
  {
    absl::MutexLock lk(&topic_filter_expression_table_mtx_);
    for (const auto& item : topic_filter_expression_table_) {
      topics.emplace_back(item.first);
    }
  }

  if (topics.empty()) {
    return;
  }

  int countdown = topics.size();
  absl::Mutex mtx;
  absl::CondVar cv;
  int acquired = 0;
  auto callback = [&](const TopicRouteDataPtr& route) {
    absl::MutexLock lk(&mtx);
    countdown--;
    cv.SignalAll();
    if (route) {
      acquired++;
    }
  };

  for (const auto& topic : topics) {
    getRouteFor(topic, callback);
  }

  while (countdown) {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }
  SPDLOG_INFO("Fetched route for {} out of {} topics", acquired, topics.size());
}

void DefaultMQPushConsumerImpl::prepareHeartbeatData(HeartbeatRequest& request) {
  auto heartbeat = new rmq::HeartbeatEntry();
  heartbeat->set_client_id(clientId());
  auto consumer_group = heartbeat->mutable_consumer_group();
  consumer_group->mutable_group()->set_arn(arn());
  consumer_group->mutable_group()->set_name(group_name_);
  consumer_group->set_consume_model(rmq::ConsumeModel::CLUSTERING);
  {
    absl::flat_hash_map<std::string, FilterExpression> topic_filter_expression_table = getTopicFilterExpressionTable();
    for (auto& filter_entry : topic_filter_expression_table) {
      auto subscription = new rmq::SubscriptionEntry;
      subscription->mutable_topic()->set_arn(arn_);
      subscription->mutable_topic()->set_name(filter_entry.first);
      subscription->mutable_expression()->set_expression(filter_entry.second.content_);
      switch (filter_entry.second.type_) {
      case ExpressionType::TAG:
        subscription->mutable_expression()->set_type(rmq::FilterType::TAG);
        break;
      case ExpressionType::SQL92:
        subscription->mutable_expression()->set_type(rmq::FilterType::SQL);
        break;
      }
      consumer_group->mutable_subscriptions()->AddAllocated(subscription);
    }

    // Add heartbeat
    request.mutable_heartbeats()->AddAllocated(heartbeat);
  }
}

ROCKETMQ_NAMESPACE_END