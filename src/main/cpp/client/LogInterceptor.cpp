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
      SPDLOG_DEBUG("Request Headers of {}:\n{}", client_rpc_info_->method(),
                   absl::StrJoin(*metadata, "\n", absl::PairFormatter(" --> ")));
    }
  }

  if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_MESSAGE)) {
    const void* message = methods->GetSendMessage();
    if (message) {
      const auto* request = reinterpret_cast<const google::protobuf::Message*>(message);
      SPDLOG_DEBUG("[Outbound] {}\n{}", client_rpc_info_->method(), request->DebugString());
    }
  }

  if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::POST_RECV_INITIAL_METADATA)) {
    std::multimap<grpc::string_ref, grpc::string_ref>* metadata = methods->GetRecvInitialMetadata();
    if (metadata) {
      absl::flat_hash_map<absl::string_view, absl::string_view> response_headers;
      for (const auto & it : *metadata) {
        response_headers.insert({absl::string_view(it.first.data(), it.first.length()),
                                 absl::string_view(it.second.data(), it.second.length())});
      }
      SPDLOG_DEBUG("Response Headers of {}:\n{}", client_rpc_info_->method(),
                   absl::StrJoin(response_headers, "\n", absl::PairFormatter(" --> ")));
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