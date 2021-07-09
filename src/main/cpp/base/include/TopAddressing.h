#pragma once

#include <cassert>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "GHttpClient.h"
#include "HostInfo.h"

ROCKETMQ_NAMESPACE_BEGIN

class TopAddressing {
public:
  TopAddressing();

  TopAddressing(std::string host, int port, std::string path);

  virtual ~TopAddressing();

  void fetchNameServerAddresses(const std::function<void(bool, const std::vector<std::string>&)>& cb);

private:
  std::string host_;
  int port_{8080};
  std::string path_;
  HostInfo host_info_;

  std::unique_ptr<GHttpClient> http_client_;
};

ROCKETMQ_NAMESPACE_END