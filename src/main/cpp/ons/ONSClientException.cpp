#include "ons/ONSClientException.h"
#include <utility>

ONS_NAMESPACE_BEGIN

ONSClientException::ONSClientException(std::string msg, int error) noexcept : msg_(std::move(msg)), error_(error) {}

ONSClientException::ONSClientException(const ONSClientException& e) noexcept {
  this->msg_ = std::string(e.what());
  this->error_ = e.GetError();
}

const char* ONSClientException::what() const noexcept { return msg_.c_str(); }

const char* ONSClientException::GetMsg() const noexcept { return msg_.c_str(); }

int ONSClientException::GetError() const noexcept { return error_; }

ONS_NAMESPACE_END