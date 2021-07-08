#include "TopicAssignmentInfo.h"
#include "spdlog/spdlog.h"

namespace rocketmq {
thread_local uint32_t TopicAssignment::query_which_broker_ = 0;

TopicAssignment::TopicAssignment(const QueryAssignmentResponse& response)
    : debug_string_(response.DebugString()) {
  if (response.common().status().code() != google::rpc::Code::OK) {
    SPDLOG_WARN("QueryAssignmentResponse#code is not SUCCESS. Keep assignment info intact. QueryAssignmentResponse: {}",
                response.DebugString());
    return;
  }

  for (const auto& item : response.assignments()) {
    const rmq::Partition& partition = item.partition();
    if (rmq::Permission::READ != partition.permission() && rmq::Permission::READ_WRITE != partition.permission()) {
      continue;
    }

    assert(partition.has_broker());
    const auto& broker = partition.broker();

    if (broker.endpoints().addresses().empty()) {
      SPDLOG_WARN("Broker[{}] is not addressable", broker.DebugString());
      continue;
    }

    MQMessageQueue message_queue(partition.topic().name(), partition.broker().name(), partition.id());
    std::string service_address;
    for (const auto& address : broker.endpoints().addresses()) {
      if (service_address.empty()) {
        switch (broker.endpoints().scheme()) {
        case rmq::AddressScheme::IPv4:
          service_address.append("ipv4:");
          break;
        case rmq::AddressScheme::IPv6:
          service_address.append("ipv6:");
          break;
        case rmq::AddressScheme::DOMAIN_NAME:
          service_address.append("dns:");
          break;
        default:
          SPDLOG_WARN("Unsupported gRPC naming scheme");
          break;
        }
      } else {
        service_address.append(",");
      }
      service_address.append(address.host()).append(":").append(std::to_string(address.port()));

      if (rmq::AddressScheme::DOMAIN_NAME == broker.endpoints().scheme()) {
        break;
      }
    }
    message_queue.serviceAddress(service_address);

    ConsumeMessageType mode;
    switch (item.mode()) {
    case rmq::ConsumeMessageType::PULL: {
      mode = ConsumeMessageType::PULL;
      break;
    }
    case rmq::ConsumeMessageType::POP: {
      mode = ConsumeMessageType::POP;
      break;
    }
    default: {
      SPDLOG_WARN("Unknown message request mode: {}, default to pop", item.mode());
      mode = ConsumeMessageType::POP;
    }
    }
    assignment_list_.emplace_back(Assignment(message_queue, mode));
  }
  std::sort(assignment_list_.begin(), assignment_list_.end());
}

} // namespace rocketmq
