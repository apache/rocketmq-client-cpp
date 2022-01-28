#include "SessionImpl.h"

#include <string>

#include "absl/memory/memory.h"
#include "apache/rocketmq/v1/service.pb.h"
#include "grpcpp/client_context.h"

ROCKETMQ_NAMESPACE_BEGIN

void SessionImpl::queryRoute(absl::flat_hash_map<std::string, std::string> metadata,
                             const rmq::QueryRouteRequest* request,
                             std::function<void(const grpc::Status&, const rmq::QueryRouteResponse&)> cb) {
  auto response = new rmq::QueryRouteResponse;
  auto client_context = new grpc::ClientContext;

  for (const auto& entry : metadata) {
    client_context->AddMetadata(entry.first, entry.second);
  }

  auto callback = [=](grpc::Status s) {
    auto reply = absl::WrapUnique(response);
    auto ctx = absl::WrapUnique(client_context);
    cb(s, *response);
  };

  stub_->async()->QueryRoute(client_context, request, response, callback);
}

ROCKETMQ_NAMESPACE_END