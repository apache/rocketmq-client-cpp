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
#include "ByteBuffer.hpp"

#include <algorithm>  // std::move

#include "DefaultByteBuffer.hpp"

namespace rocketmq {

ByteBuffer* ByteBuffer::allocate(int32_t capacity) {
  if (capacity < 0) {
    throw std::invalid_argument("");
  }
  return new DefaultByteBuffer(capacity, capacity);
}

ByteBuffer* ByteBuffer::wrap(ByteArrayRef array, int32_t offset, int32_t length) {
  try {
    return new DefaultByteBuffer(std::move(array), offset, length);
  } catch (const std::exception& x) {
    throw std::runtime_error("IndexOutOfBoundsException");
  }
}

}  // namespace rocketmq
