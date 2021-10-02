#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "CommandCustomHeader.h"
#include "LanguageCode.h"
#include "RequestCode.h"
#include "ResponseCode.h"
#include "Version.h"

ROCKETMQ_NAMESPACE_BEGIN

/**
 * RemotingCommand is non-copyable. It is movable.
 */
class RemotingCommand {
public:
  RemotingCommand(const RemotingCommand&) = delete;

  RemotingCommand(RemotingCommand&& rhs) noexcept {
    code_ = rhs.code_;
    language_ = rhs.language_;
    version_ = rhs.version_;
    opaque_ = rhs.opaque_;
    remark_ = std::move(rhs.remark_);
    body_ = std::move(rhs.body_);
    ext_fields_ = rhs.ext_fields_;
    rhs.ext_fields_ = nullptr;
  }

  RemotingCommand& operator=(const RemotingCommand&) = delete;

  RemotingCommand& operator=(RemotingCommand&& rhs) noexcept {
    if (this == &rhs) {
      return *this;
    }

    code_ = rhs.code_;
    language_ = rhs.language_;
    version_ = rhs.version_;
    opaque_ = rhs.opaque_;
    remark_ = std::move(rhs.remark_);
    body_ = std::move(rhs.body_);
    ext_fields_ = rhs.ext_fields_;
    rhs.ext_fields_ = nullptr;
    return *this;
  }

  virtual ~RemotingCommand() {
    delete ext_fields_;
  }

  static std::int32_t nextRequestId();

  static RemotingCommand createRequest(RequestCode, CommandCustomHeader*);

  static RemotingCommand createResponse(ResponseCode, CommandCustomHeader*);

  virtual void encodeHeader(google::protobuf::Value& root);

private:
  RemotingCommand() = default;

  std::int32_t code_{static_cast<std::int32_t>(RequestCode::QueryRoute)};
  LanguageCode language_{LanguageCode::CPP};
  std::int32_t version_{static_cast<std::int32_t>(Version::V4_9_1)};
  std::int32_t opaque_{nextRequestId()};
  std::uint32_t flag_{0};
  std::string remark_;

  CommandCustomHeader* ext_fields_{nullptr};

  std::vector<std::uint8_t> body_;

  /**
   * Bit-field shift amount for flag_ field, indicating the RPC is a response from broker or name-server.
   */
  const static std::uint8_t RPC_TYPE_RESPONSE;

  /**
   * Bit-field shift amount for flag_ field. Request marked one-way should NOT expect a respose from broker or
   * name-server.
   */
  const static std::uint8_t RPC_TYPE_ONE_WAY;
};

ROCKETMQ_NAMESPACE_END