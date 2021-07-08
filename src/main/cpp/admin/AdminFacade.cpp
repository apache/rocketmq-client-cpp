#include "AdminServerImpl.h"

using namespace ROCKETMQ_NAMESPACE::admin;

AdminServer& AdminFacade::getServer() {
  static AdminServerImpl admin_server;
  return admin_server;
}