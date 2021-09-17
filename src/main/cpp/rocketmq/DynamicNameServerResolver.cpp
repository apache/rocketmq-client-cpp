#include "DynamicNameServerResolver.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

#include "absl/strings/str_join.h"
#include "spdlog/spdlog.h"

#include "SchedulerImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

DynamicNameServerResolver::DynamicNameServerResolver(absl::string_view endpoint,
                                                     std::chrono::milliseconds refresh_interval)
    : endpoint_(endpoint.data(), endpoint.length()), refresh_interval_(refresh_interval),
      scheduler_(absl::make_unique<SchedulerImpl>()) {
  absl::string_view remains;
  if (absl::StartsWith(endpoint_, "https://")) {
    ssl_ = true;
    remains = absl::StripPrefix(endpoint_, "https://");
  } else {
    remains = absl::StripPrefix(endpoint_, "http://");
  }

  std::int32_t port = 80;
  if (ssl_) {
    port = 443;
  }

  absl::string_view host;
  if (absl::StrContains(remains, ':')) {
    std::vector<absl::string_view> segments = absl::StrSplit(remains, ':');
    host = segments[0];
    remains = absl::StripPrefix(remains, host);
    remains = absl::StripPrefix(remains, ":");

    segments = absl::StrSplit(remains, '/');
    if (!absl::SimpleAtoi(segments[0], &port)) {
      SPDLOG_WARN("Failed to parse port of name-server-list discovery service endpoint");
      abort();
    }
    remains = absl::StripPrefix(remains, segments[0]);
  } else {
    std::vector<absl::string_view> segments = absl::StrSplit(remains, '/');
    host = segments[0];
    remains = absl::StripPrefix(remains, host);
  }

  top_addressing_ = absl::make_unique<TopAddressing>(std::string(host.data(), host.length()), port,
                                                     std::string(remains.data(), remains.length()));
}

std::vector<std::string> DynamicNameServerResolver::resolve() {
  bool fetch_immediately = false;
  {
    absl::MutexLock lk(&name_server_list_mtx_);
    if (name_server_list_.empty()) {
      fetch_immediately = true;
    }
  }

  if (fetch_immediately) {
    fetch();
  }

  {
    absl::MutexLock lk(&name_server_list_mtx_);
    return name_server_list_;
  }
}

void DynamicNameServerResolver::fetch() {
  std::weak_ptr<DynamicNameServerResolver> ptr(shared_from_this());
  auto callback = [ptr](bool success, const std::vector<std::string>& name_server_list) {
    if (success && !name_server_list.empty()) {
      std::shared_ptr<DynamicNameServerResolver> resolver = ptr.lock();
      if (resolver) {
        resolver->onNameServerListFetched(name_server_list);
      }
    }
  };
  top_addressing_->fetchNameServerAddresses(callback);
}

void DynamicNameServerResolver::onNameServerListFetched(const std::vector<std::string>& name_server_list) {
  if (!name_server_list.empty()) {
    absl::MutexLock lk(&name_server_list_mtx_);
    if (name_server_list_ != name_server_list) {
      SPDLOG_INFO("Name server list changed. {} --> {}", absl::StrJoin(name_server_list_, ";"),
                  absl::StrJoin(name_server_list, ";"));
      name_server_list_ = name_server_list;
    }
  }
}

void DynamicNameServerResolver::injectHttpClient(std::unique_ptr<HttpClient> http_client) {
  top_addressing_->injectHttpClient(std::move(http_client));
}

void DynamicNameServerResolver::start() {
  scheduler_->start();
  scheduler_->schedule(std::bind(&DynamicNameServerResolver::fetch, this), "DynamicNameServerResolver",
                       std::chrono::milliseconds(0), refresh_interval_);
}

void DynamicNameServerResolver::shutdown() { scheduler_->shutdown(); }

std::string DynamicNameServerResolver::current() {
  absl::MutexLock lk(&name_server_list_mtx_);
  if (name_server_list_.empty()) {
    return std::string();
  }

  std::uint32_t index = index_.load(std::memory_order_relaxed) % name_server_list_.size();
  return name_server_list_[index];
}

std::string DynamicNameServerResolver::next() {
  index_.fetch_add(1, std::memory_order_relaxed);
  return current();
}

ROCKETMQ_NAMESPACE_END