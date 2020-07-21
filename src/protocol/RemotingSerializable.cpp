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
#include "RemotingSerializable.h"

#include <memory>
#include <sstream>

#include "MQException.h"

namespace rocketmq {

Json::StreamWriterBuilder& RemotingSerializable::getPrettyWriterBuilder() {
  static Json::StreamWriterBuilder sPrettyWriterBuilder;
  return sPrettyWriterBuilder;
}

Json::StreamWriterBuilder& RemotingSerializable::getPlainWriterBuilder() {
  static PlainStreamWriterBuilder sPlainWriterBuilder;
  return sPlainWriterBuilder;
}

Json::CharReaderBuilder& RemotingSerializable::getPowerReaderBuilder() {
  static PowerCharReaderBuilder sPowerReaderBuilder;
  return sPowerReaderBuilder;
}

std::string RemotingSerializable::toJson(Json::Value root) {
  return toJson(root, false);
}

std::string RemotingSerializable::toJson(Json::Value root, bool prettyFormat) {
  std::ostringstream sout;
  toJson(root, sout, prettyFormat);
  return sout.str();
}

void RemotingSerializable::toJson(Json::Value root, std::ostream& sout, bool prettyFormat) {
  std::unique_ptr<Json::StreamWriter> writer;
  if (prettyFormat) {
    writer.reset(getPrettyWriterBuilder().newStreamWriter());
  } else {
    writer.reset(getPlainWriterBuilder().newStreamWriter());
  }
  writer->write(root, &sout);
}

Json::Value RemotingSerializable::fromJson(std::istream& sin) {
  std::ostringstream ssin;
  ssin << sin.rdbuf();
  std::string json = ssin.str();
  return fromJson(json);
}

Json::Value RemotingSerializable::fromJson(const std::string& json) {
  const char* begin = json.data();
  const char* end = begin + json.size();
  return fromJson(begin, end);
}

Json::Value RemotingSerializable::fromJson(const ByteArray& bytes) {
  const char* begin = bytes.array();
  const char* end = begin + bytes.size();
  return fromJson(begin, end);
}

Json::Value RemotingSerializable::fromJson(const char* begin, const char* end) {
  Json::Value root;
  std::string errs;
  std::unique_ptr<Json::CharReader> reader(getPowerReaderBuilder().newCharReader());
  // TODO: object as key
  if (reader->parse(begin, end, &root, &errs)) {
    return root;
  } else {
    THROW_MQEXCEPTION(RemotingCommandException, errs, -1);
  }
}

}  // namespace rocketmq
