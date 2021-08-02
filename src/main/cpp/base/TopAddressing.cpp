#include "TopAddressing.h"

#include "GHttpClient.h"
#include "absl/memory/memory.h"
#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "spdlog/spdlog.h"
#include <utility>

ROCKETMQ_NAMESPACE_BEGIN

TopAddressing::TopAddressing() : TopAddressing("jmenv.tbsite.net", 8080, "/rocketmq/nsaddr") {}

TopAddressing::TopAddressing(std::string host, int port, std::string path)
    : host_(std::move(host)), port_(port), path_(std::move(path)), http_client_(absl::make_unique<GHttpClient>()) {
  http_client_->start();
}

TopAddressing::~TopAddressing() { http_client_->shutdown(); }

void TopAddressing::fetchNameServerAddresses(const std::function<void(bool, const std::vector<std::string>&)>& cb) {
  SPDLOG_DEBUG("Prepare to send HTTP request, timeout=3s");
  std::string base(fmt::format("http://{}:{}", host_, port_));
  // Append host info if necessary.
  std::string query_string(path_);

  if (absl::StrContains(query_string, "?")) {
    query_string.append("&");
  } else {
    query_string.append("?");
  }

  if (host_info_.hasHostInfo()) {
    query_string.append(host_info_.queryString());
  } else {
    query_string.append("nofix=1");
  }

  auto callback = [cb](int code, const absl::flat_hash_map<std::string, std::string>& metadata,
                       const std::string& body) {
    SPDLOG_DEBUG("Receive HTTP response. Code: {}, body: {}", code, body);
    if (GHttpClient::STATUS_OK == code) {
      cb(true, absl::StrSplit(body, ';'));
    } else {
      std::vector<std::string> name_server_list;
      cb(false, name_server_list);
    }
  };

  http_client_->get(HttpProtocol::HTTP, host_, port_, query_string, callback);
}

void TopAddressing::injectHttpClient(std::unique_ptr<HttpClient> http_client) {
  http_client_->shutdown();
  http_client_.swap(http_client);
}

ROCKETMQ_NAMESPACE_END