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
#ifndef __MQ_CLIENT_EXCEPTION_H__
#define __MQ_CLIENT_EXCEPTION_H__

#include <exception>
#include <ostream>
#include <sstream>
#include <string>

#include "RocketMQClient.h"

namespace rocketmq {

class ROCKETMQCLIENT_API MQException : public std::exception {
 public:
  MQException(const std::string& msg, int error, const char* file, int line) noexcept
      : m_error(error), m_line(line), m_msg(msg), m_file(file), m_type("MQException") {}

  MQException(const std::string& msg, int error, const char* file, const char* type, int line) noexcept
      : m_error(error), m_line(line), m_msg(msg), m_file(file), m_type(type) {}

  virtual ~MQException() noexcept = default;

  const char* what() const noexcept override {
    if (m_what_.empty()) {
      std::stringstream ss;
      ss << "[" << m_type << "] msg: " << m_msg << ", error: " << m_error << ", in <" << m_file << ":" << m_line << ">";
      m_what_ = ss.str();
    }
    return m_what_.c_str();
  }

  int GetError() const noexcept { return m_error; }
  int GetLine() { return m_line; }
  const char* GetMsg() { return m_msg.c_str(); }
  const char* GetFile() { return m_file.c_str(); }
  const char* GetType() const noexcept { return m_type.c_str(); }

 protected:
  int m_error;
  int m_line;
  std::string m_msg;
  std::string m_file;
  std::string m_type;

  mutable std::string m_what_;
};

inline std::ostream& operator<<(std::ostream& os, const MQException& e) {
  os << e.what();
  return os;
}

#define DEFINE_MQCLIENTEXCEPTION2(name, super)                                                     \
  class ROCKETMQCLIENT_API name : public super {                                                   \
   public:                                                                                         \
    name(const std::string& msg, int error, const char* file, int line) noexcept                   \
        : super(msg, error, file, #name, line) {}                                                  \
                                                                                                   \
   protected:                                                                                      \
    name(const std::string& msg, int error, const char* file, const char* type, int line) noexcept \
        : super(msg, error, file, type, line) {}                                                   \
  };

#define DEFINE_MQCLIENTEXCEPTION(name) DEFINE_MQCLIENTEXCEPTION2(name, MQException)

DEFINE_MQCLIENTEXCEPTION(MQClientException)
DEFINE_MQCLIENTEXCEPTION(MQBrokerException)
DEFINE_MQCLIENTEXCEPTION(InterruptedException)
DEFINE_MQCLIENTEXCEPTION(RemotingException)
DEFINE_MQCLIENTEXCEPTION2(RemotingCommandException, RemotingException)
DEFINE_MQCLIENTEXCEPTION2(RemotingConnectException, RemotingException)
DEFINE_MQCLIENTEXCEPTION2(RemotingSendRequestException, RemotingException)
DEFINE_MQCLIENTEXCEPTION2(RemotingTimeoutException, RemotingException)
DEFINE_MQCLIENTEXCEPTION2(RemotingTooMuchRequestException, RemotingException)
DEFINE_MQCLIENTEXCEPTION(UnknownHostException)

#define THROW_MQEXCEPTION(e, msg, err) throw e(msg, err, __FILE__, __LINE__)
#define NEW_MQEXCEPTION(e, msg, err) e(msg, err, __FILE__, __LINE__)

}  // namespace rocketmq

#endif  // __MQ_CLIENT_EXCEPTION_H__
