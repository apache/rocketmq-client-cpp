#pragma once

#include "State.h"

namespace rocketmq {
namespace admin {

  class AdminServer {
  public:
    virtual ~AdminServer() = default;

    virtual bool start() = 0;

    virtual bool stop() = 0;

    virtual int port() const = 0;
  };

  class AdminFacade {
  public:
    static AdminServer& getServer();
  };

} // namespace admin
} // namespace rocketmq