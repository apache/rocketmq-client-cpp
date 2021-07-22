#include "GHttpClient.h"
#include "absl/strings/str_join.h"
#include "fmt/format.h"
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <functional>
#include <string>
#include <thread>

#include "LoggerImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

GHttpClient::GHttpClient() : shutdown_(false) {
  grpc_core::ExecCtx exec_ctx;
  grpc_httpcli_context_init(&http_context_);
  grpc_pollset* pollset = static_cast<grpc_pollset*>(gpr_zalloc(grpc_pollset_size()));
  grpc_pollset_init(pollset, &http_mtx_);
  http_polling_entity_ = grpc_polling_entity_create_from_pollset(pollset);
  GRPC_CLOSURE_INIT(&destroy_, &GHttpClient::destroyPollingEntity, &http_polling_entity_, grpc_schedule_on_exec_ctx);
}

GHttpClient::~GHttpClient() {
  SPDLOG_INFO("GHttpClient::~GHttpClient() starts");
  shutdown();
  grpc_core::ExecCtx exec_ctx;
  grpc_httpcli_context_destroy(&http_context_);
  grpc_pollset_shutdown(grpc_polling_entity_pollset(&http_polling_entity_), &destroy_);
  SPDLOG_INFO("GHttpClient::~GHttpClient() completed");
}

const int64_t GHttpClient::POLL_INTERVAL = 1000;

const int GHttpClient::STATUS_OK = 200;

void GHttpClient::poll() {
  grpc_core::ExecCtx exec_ctx;
  while (!shutdown_) {
    gpr_mu_lock(http_mtx_);
    grpc_error_handle error = grpc_pollset_work(grpc_polling_entity_pollset(&http_polling_entity_), &worker_,
                                                grpc_core::ExecCtx::Get()->Now() + POLL_INTERVAL);
    gpr_mu_unlock(http_mtx_);
    if (error) {
      SPDLOG_WARN("grpc_pollset_work failed");
    } else {
      SPDLOG_TRACE("grpc_pollset_work returned. grpc_pollset_size: {}", grpc_pollset_size());
    }
    submit0();
  }
  SPDLOG_INFO("GHttpClient::poll completed");
}

void GHttpClient::start() {
  SPDLOG_INFO("GHttpClient::start()");
  if (!shutdown_) {
    loop_ = std::thread(std::bind(&GHttpClient::poll, this));
    SPDLOG_INFO("GHttpClient starts to poll");
  }
}

void GHttpClient::shutdown() {
  bool expected = false;
  if (shutdown_.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
    SPDLOG_INFO("GHttpClient::shutdown()");
    if (loop_.joinable()) {
      loop_.join();
      SPDLOG_INFO("GHttpClient#loop thread quit OK");
    }
  }
}

void GHttpClient::get(
    HttpProtocol protocol, const std::string& host, std::uint16_t port, const std::string& path,
    const std::function<void(int, const absl::flat_hash_map<std::string, std::string>&, const std::string&)>& cb) {
  auto http_invocation_context = new HttpInvocationContext();
  std::string http_host = fmt::format("{}:{}", host, port);
  http_invocation_context->host = http_host;
  http_invocation_context->path = path;
  http_invocation_context->request.host = const_cast<char*>(http_invocation_context->host.c_str());
  http_invocation_context->request.http.path = const_cast<char*>(http_invocation_context->path.c_str());
  http_invocation_context->request.handshaker = &grpc_httpcli_plaintext;
  http_invocation_context->callback = cb;

  {
    absl::MutexLock lk(&pending_requests_mtx_);
    pending_requests_.emplace_back(http_invocation_context);
    SPDLOG_TRACE("Add HTTP request to pending list");
  }

  {
    grpc_core::ExecCtx exec_ctx;
    SPDLOG_TRACE("Prepare to grpc_pollset_kick");
    {
      gpr_mu_lock(http_mtx_);
      grpc_error_handle error = grpc_pollset_kick(grpc_polling_entity_pollset(&http_polling_entity_), nullptr);
      gpr_mu_unlock(http_mtx_);
      if (GRPC_ERROR_NONE != error) {
        SPDLOG_WARN("grpc_pollset_kick failed");
      } else {
        SPDLOG_TRACE("grpc_pollset_kick completed");
      }
    }
  }
}

void GHttpClient::submit0() {
  absl::MutexLock lk(&pending_requests_mtx_);
  if (pending_requests_.empty()) {
    SPDLOG_TRACE("No pending HTTP requests");
    return;
  }

  SPDLOG_DEBUG("Add {} pending HTTP requests to pollset", pending_requests_.size());
  grpc_core::ExecCtx exec_ctx;
  {
    for (auto it = pending_requests_.begin(); it != pending_requests_.end();) {
      auto http_invocation_context = *it;
      SPDLOG_TRACE("Prepare to create quota");
      grpc_resource_quota* resource_quota = grpc_resource_quota_create("get");
      SPDLOG_TRACE("Quota created");
      SPDLOG_TRACE("grpc_httpcli_get starts");
      grpc_httpcli_get(
          &http_context_, &http_polling_entity_, resource_quota, &http_invocation_context->request,
          grpc_core::ExecCtx::Get()->Now() + absl::ToInt64Milliseconds(absl::Seconds(3)),
          GRPC_CLOSURE_CREATE(&GHttpClient::onCompletion, http_invocation_context, grpc_schedule_on_exec_ctx),
          &http_invocation_context->response);
      SPDLOG_TRACE("grpc_httpcli_get completed");
      grpc_resource_quota_unref_internal(resource_quota);
      SPDLOG_TRACE("Resource quota unref completed");
      it = pending_requests_.erase(it);
    }
  }
}

void GHttpClient::destroyPollingEntity(void* arg, grpc_error_handle error) {
  auto polling_entity = reinterpret_cast<grpc_polling_entity*>(arg);
  grpc_pollset_destroy(grpc_polling_entity_pollset(polling_entity));
}

void GHttpClient::onCompletion(void* arg, grpc_error_handle error) {
  auto context = reinterpret_cast<HttpInvocationContext*>(arg);
  absl::flat_hash_map<std::string, std::string> metadata;

  if (error) {
    context->callback(500, metadata, std::string());
    SPDLOG_WARN("HTTP request failed");
    delete context;
    return;
  }

  for (size_t i = 0; i < context->response.hdr_count; i++) {
    grpc_http_header* hdr = context->response.hdrs + i;
    metadata.insert({std::string(hdr->key), std::string(hdr->value)});
  }
  std::string body(context->response.body, context->response.body_length);
  SPDLOG_DEBUG("HTTP response received. Code: {}, response-headers: {}, response-body: {}", context->response.status,
               absl::StrJoin(metadata, ",", absl::PairFormatter("=")), body);
  context->callback(context->response.status, metadata, body);
  delete context;
}

ROCKETMQ_NAMESPACE_END