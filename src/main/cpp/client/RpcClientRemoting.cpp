#include "RpcClientRemoting.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>

#include "ResponseCode.h"
#include "absl/strings/str_join.h"
#include "spdlog/spdlog.h"

#include "QueryRouteRequestHeader.h"

ROCKETMQ_NAMESPACE_BEGIN

void RpcClientRemoting::connect() {
  std::weak_ptr<RpcClientRemoting> self = shared_from_this();
  auto callback = std::bind(&RpcClientRemoting::onCallback, self, std::placeholders::_1);
  session_ = std::make_shared<RemotingSession>(context_, endpoint_, callback);

  // Use blocking connect in development phase
  session_->connect(std::chrono::seconds(3), true);
}

bool RpcClientRemoting::ok() const {
  if (!session_) {
    return false;
  }

  return session_->state() == SessionState::Connected;
}

void RpcClientRemoting::asyncQueryRoute(const QueryRouteRequest& request,
                                        InvocationContext<QueryRouteResponse>* invocation_context) {
  assert(invocation_context);

  // Assign RequestCode
  invocation_context->request_code = RequestCode::QueryRoute;

  auto header = new QueryRouteRequestHeader();
  if (request.topic().resource_namespace().empty()) {
    header->topic(request.topic().name());
  } else {
    header->topic(absl::StrJoin({request.topic().resource_namespace(), request.topic().name()}, "%"));
  }

  auto command = RemotingCommand::createRequest(RequestCode::QueryRoute, header);

  {
    absl::MutexLock lk(&in_flight_requests_mtx_);
    in_flight_requests_.insert({command.opaque(), invocation_context});
  }
}

void RpcClientRemoting::onCallback(std::weak_ptr<RpcClientRemoting> rpc_client,
                                   const std::vector<RemotingCommand>& commands) {
  std::shared_ptr<RpcClientRemoting> remoting_client = rpc_client.lock();
  if (!remoting_client) {
    return;
  }

  for (const auto& command : commands) {
    remoting_client->processCommand(command);
  }
}

void RpcClientRemoting::processCommand(const RemotingCommand& command) {
  std::int32_t opaque = command.opaque();
  BaseInvocationContext* invocation_context = nullptr;
  {
    absl::MutexLock lk(&in_flight_requests_mtx_);
    if (in_flight_requests_.contains(opaque)) {
      invocation_context = in_flight_requests_[opaque];
      in_flight_requests_.erase(opaque);
    }
  }

  if (!invocation_context) {
    SPDLOG_WARN("Failed to look-up invocation-context through opaque[{}]", opaque);
    return;
  }

  switch (invocation_context->request_code) {
    case RequestCode::QueryRoute: {
      auto context = dynamic_cast<InvocationContext<QueryRouteResponse>*>(invocation_context);
      assert(nullptr != context);
      ResponseCode code = static_cast<ResponseCode>(command.code());
      switch (code) {
        case ResponseCode::Success: {
          google::protobuf::Struct root;
          auto data_ptr = reinterpret_cast<const char*>(command.body().data());
          std::string json(data_ptr, command.body().size());
          /*
           * Sample JSON:
           * {
           *  "brokerDatas":[{"brokerAddrs":{0:"100.82.112.185:10911"},"brokerName":"metaq-yuan-test-1","cluster":"metaq-yuan-test"}],
           *  "filterServerTable":{},
           *  "queueDatas":[{"brokerName":"metaq-yuan-test-1","perm":6,"readQueueNums":8,"topicSynFlag":0,"writeQueueNums":8}]
           * }
           */
          auto status = google::protobuf::util::JsonStringToMessage(json, &root);
          if (!status.ok()) {
            SPDLOG_WARN("Failed to parse JSON: {}. Cause: {}", json, status.message().as_string());
            return;
          }
          break;
        }
        case ResponseCode::InternalSystemError: {
          break;
        }
        case ResponseCode::TooManyRequests: {
          break;
        }
      }
      break;
    }

    case RequestCode::SendMessage: {
      break;
    }

    case RequestCode::PopMessage: {
      break;
    }

    case RequestCode::AckMessage: {
      break;
    }

    case RequestCode::PullMessage: {
      break;
    }

    case RequestCode::Absent: {
      break;
    }
  }
}

ROCKETMQ_NAMESPACE_END