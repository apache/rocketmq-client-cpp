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
#ifndef ROCKETMQ_PROTOCOL_REMOTINGSERIALIZABLE_H_
#define ROCKETMQ_PROTOCOL_REMOTINGSERIALIZABLE_H_

#include <json/json.h>

#include "ByteArray.h"

namespace rocketmq {

class RemotingSerializable {
 public:
  virtual ~RemotingSerializable() = default;

  virtual std::string encode() { return ""; }

  static std::string toJson(Json::Value root);
  static std::string toJson(Json::Value root, bool prettyFormat);
  static void toJson(Json::Value root, std::ostream& sout, bool prettyFormat);

  static Json::Value fromJson(std::istream& sin);
  static Json::Value fromJson(const std::string& json);
  static Json::Value fromJson(const ByteArray& bytes);
  static Json::Value fromJson(const char* begin, const char* end);

 private:
  class PlainStreamWriterBuilder : public Json::StreamWriterBuilder {
   public:
    PlainStreamWriterBuilder() : StreamWriterBuilder() { (*this)["indentation"] = ""; }
  };

  class PowerCharReaderBuilder : public Json::CharReaderBuilder {
   public:
    PowerCharReaderBuilder() : CharReaderBuilder() { (*this)["allowNumericKeys"] = true; }
  };

  static Json::StreamWriterBuilder& getPrettyWriterBuilder();
  static Json::StreamWriterBuilder& getPlainWriterBuilder();
  static Json::CharReaderBuilder& getPowerReaderBuilder();
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PROTOCOL_REMOTINGSERIALIZABLE_H_
