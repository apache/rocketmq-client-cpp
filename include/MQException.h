/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ROCKETMQ_MQEXCEPTION_H_
#define ROCKETMQ_MQEXCEPTION_H_

#include <exception>
#include <ostream>
#include <sstream>
#include <string>

#include "RocketMQClient.h"

namespace rocketmq {

/**
 * MQException - base exception
 */
class ROCKETMQCLIENT_API MQException : public std::exception {
 public:
  MQException(const std::string& msg, int error, const char* file, int line) noexcept
      : MQException(msg, error, nullptr, file, line) {}

  MQException(const std::string& msg, int error, std::exception_ptr cause, const char* file, int line) noexcept
      : MQException("MQException", msg, error, cause, file, line) {}

  MQException(const std::string& type,
              const std::string& msg,
              int error,
              std::exception_ptr cause,
              const char* file,
              int line) noexcept : type_(type),
                                   msg_(msg),
                                   error_(error),
                                   cause_(cause),
                                   file_(file),
                                   line_(line) {}

  virtual ~MQException() noexcept = default;

  const char* what() const noexcept override {
    if (what_.empty()) {
      std::stringstream ss;
      ss << "[" << type_ << "] msg: " << msg_ << ", error: " << error_ << ", in <" << file_ << ":" << line_ << ">";
      what_ = ss.str();
    }
    return what_.c_str();
  }

  const char* GetType() const noexcept { return type_.c_str(); }

  const std::string& GetErrorMessage() const noexcept { return msg_; }
  const char* GetMsg() const noexcept { return msg_.c_str(); }

  int GetError() const noexcept { return error_; }

  std::exception_ptr GetCause() const { return cause_; }

  const char* GetFile() const noexcept { return file_.c_str(); }
  int GetLine() const noexcept { return line_; }

 protected:
  std::string type_;
  std::string msg_;
  int error_;

  std::exception_ptr cause_;

  std::string file_;
  int line_;

  mutable std::string what_;
};

inline std::ostream& operator<<(std::ostream& os, const MQException& e) {
  os << e.what();
  return os;
}

#define DEFINE_MQEXCEPTION2(name, super)                                                                   \
  class ROCKETMQCLIENT_API name : public super {                                                           \
   public:                                                                                                 \
    name(const std::string& msg, int error, const char* file, int line) noexcept                           \
        : name(msg, error, nullptr, file, line) {}                                                         \
    name(const std::string& msg, int error, std::exception_ptr cause, const char* file, int line) noexcept \
        : name(#name, msg, error, cause, file, line) {}                                                    \
                                                                                                           \
   protected:                                                                                              \
    name(const std::string& type,                                                                          \
         const std::string& msg,                                                                           \
         int error,                                                                                        \
         std::exception_ptr cause,                                                                         \
         const char* file,                                                                                 \
         int line) noexcept : super(type, msg, error, cause, file, line) {}                                \
  };

#define DEFINE_MQEXCEPTION(name) DEFINE_MQEXCEPTION2(name, MQException)

DEFINE_MQEXCEPTION(MQClientException)
DEFINE_MQEXCEPTION(MQBrokerException)
DEFINE_MQEXCEPTION(InterruptedException)
DEFINE_MQEXCEPTION(RemotingException)
DEFINE_MQEXCEPTION2(RemotingCommandException, RemotingException)
DEFINE_MQEXCEPTION2(RemotingConnectException, RemotingException)
DEFINE_MQEXCEPTION2(RemotingSendRequestException, RemotingException)
DEFINE_MQEXCEPTION2(RemotingTimeoutException, RemotingException)
DEFINE_MQEXCEPTION2(RemotingTooMuchRequestException, RemotingException)
DEFINE_MQEXCEPTION(UnknownHostException)
DEFINE_MQEXCEPTION(RequestTimeoutException)

#define THROW_MQEXCEPTION(e, msg, err) throw e((msg), (err), __FILE__, __LINE__)
#define THROW_MQEXCEPTION2(e, msg, err, cause) throw e((msg), (err), (cause), __FILE__, __LINE__)

#define NEW_MQEXCEPTION(e, msg, err) e((msg), (err), __FILE__, __LINE__)
#define NEW_MQEXCEPTION2(e, msg, err, cause) e((msg), (err), (cause), __FILE__, __LINE__)

}  // namespace rocketmq

#endif  // ROCKETMQ_MQEXCEPTION_H_
