#pragma once

#include <exception>
#include <string>

#include "ONSClient.h"
#include "ONSErrorCode.h"

ONS_NAMESPACE_BEGIN

class ONSCLIENT_API ONSClientException : public std::exception {
public:
  ONSClientException() = delete;
  ONSClientException(const ONSClientException& e) noexcept;
  ~ONSClientException() noexcept override = default;
  explicit ONSClientException(std::string msg, int code = OTHER_ERROR) noexcept;

  const char* GetMsg() const noexcept;
  const char* what() const noexcept override;
  int GetError() const noexcept;

private:
  std::string msg_;
  int error_{};
};

#define THROW_ONS_EXCEPTION(e, msg, code) throw e(msg, code)

ONS_NAMESPACE_END