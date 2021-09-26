#include "LogInterceptor.h"
#include "InterceptorContinuation.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_join.h"
#include "google/protobuf/message.h"
#include "rocketmq/Logger.h"
#include "spdlog/spdlog.h"

ROCKETMQ_NAMESPACE_BEGIN

void LogInterceptor::Intercept(grpc::experimental::InterceptorBatchMethods* methods) {
  InterceptorContinuation continuation(methods);

  if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_INITIAL_METADATA)) {
    std::multimap<std::string, std::string>* metadata = methods->GetSendInitialMetadata();
    if (metadata) {
      SPDLOG_DEBUG("[Outbound]Headers of {}: \n{}", client_rpc_info_->method(),
                   absl::StrJoin(*metadata, "\n", absl::PairFormatter(" --> ")));
    }
  }

  if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_MESSAGE)) {
    grpc::ByteBuffer* buffer = methods->GetSerializedSendMessage();
    if (buffer) {
      SPDLOG_DEBUG("[Outbound] {}: Buffer: {}bytes", client_rpc_info_->method(), buffer->Length());
    }
  }

  if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::POST_RECV_INITIAL_METADATA)) {
    std::multimap<grpc::string_ref, grpc::string_ref>* metadata = methods->GetRecvInitialMetadata();
    if (metadata) {
      absl::flat_hash_map<absl::string_view, absl::string_view> response_headers;
      for (const auto& it : *metadata) {
        response_headers.insert({absl::string_view(it.first.data(), it.first.length()),
                                 absl::string_view(it.second.data(), it.second.length())});
      }
      if (!response_headers.empty()) {
        SPDLOG_DEBUG("[Inbound]Response Headers of {}:\n{}", client_rpc_info_->method(),
                     absl::StrJoin(response_headers, "\n", absl::PairFormatter(" --> ")));
      } else {
        SPDLOG_DEBUG("[Inbound]Response metadata of {} is empty", client_rpc_info_->method());
      }
    }
  }

  if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::POST_RECV_MESSAGE)) {
    void* message = methods->GetRecvMessage();
    if (message) {
      auto* response = reinterpret_cast<google::protobuf::Message*>(message);
      SPDLOG_DEBUG("[Inbound] {}\n{}", client_rpc_info_->method(), response->DebugString());
    }
  }
}

ROCKETMQ_NAMESPACE_END