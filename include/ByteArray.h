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
#ifndef ROCKETMQ_BYTEARRAY_H_
#define ROCKETMQ_BYTEARRAY_H_

#include <memory>  // std::shared_ptr

#include "Array.h"

namespace rocketmq {

typedef Array<char> ByteArray;
typedef std::shared_ptr<Array<char>> ByteArrayRef;

ByteArrayRef slice(ByteArrayRef ba, size_t offset);
ByteArrayRef slice(ByteArrayRef ba, size_t offset, size_t size);

ByteArrayRef stoba(const std::string& str);
ByteArrayRef stoba(std::string&& str);

ByteArrayRef catoba(const char* str, size_t len);

std::string batos(ByteArrayRef ba);

}  // namepace rocketmq

#endif  // ROCKETMQ_BYTEARRAY_H_
