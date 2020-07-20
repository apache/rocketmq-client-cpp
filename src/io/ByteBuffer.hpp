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
#ifndef ROCKETMQ_IO_BYTEBUFFER_HPP_
#define ROCKETMQ_IO_BYTEBUFFER_HPP_

#include <sstream>  // std::stringstream

#include "ByteArray.h"
#include "Buffer.hpp"
#include "ByteOrder.h"

namespace rocketmq {

class ByteBuffer : public Buffer<char> {
 public:
  static ByteBuffer* allocate(int32_t capacity);

  static ByteBuffer* wrap(ByteArrayRef array, int32_t offset, int32_t length);
  static ByteBuffer* wrap(ByteArrayRef array) { return wrap(array, 0, array->size()); }

 protected:
  ByteBuffer(int32_t mark, int32_t pos, int32_t lim, int32_t cap, ByteArrayRef byte_array, int32_t offset)
      : Buffer(mark, pos, lim, cap),
        byte_array_(std::move(byte_array)),
        offset_(offset),
        is_read_only_(false),
        big_endian_(true) {}

  ByteBuffer(int32_t mark, int32_t pos, int32_t lim, int32_t cap) : ByteBuffer(mark, pos, lim, cap, nullptr, 0) {}

 public:
  virtual ~ByteBuffer() = default;

 public:
  virtual ByteBuffer* slice() = 0;
  virtual ByteBuffer* duplicate() = 0;
  virtual ByteBuffer* asReadOnlyBuffer() = 0;

  // get/put routine
  // ======================

  virtual char get() = 0;
  virtual ByteBuffer& put(char b) = 0;

  virtual char get(int32_t index) const = 0;
  virtual ByteBuffer& put(int32_t index, char b) = 0;

  virtual ByteBuffer& get(ByteArray& dst, int32_t offset, int32_t length) {
    checkBounds(offset, length, dst.size());
    if (length > remaining()) {
      throw std::runtime_error("BufferUnderflowException");
    }
    int32_t end = offset + length;
    for (int32_t i = offset; i < end; i++) {
      dst[i] = get();
    }
    return *this;
  }

  ByteBuffer& get(ByteArray& dst) { return get(dst, 0, dst.size()); }

  virtual ByteBuffer& put(ByteBuffer& src) {
    if (&src == this) {
      throw std::invalid_argument("");
    }
    if (isReadOnly()) {
      throw std::runtime_error("ReadOnlyBufferException");
    }
    int32_t n = src.remaining();
    if (n > remaining()) {
      throw std::runtime_error("BufferOverflowException");
    }
    for (int32_t i = 0; i < n; i++) {
      put(src.get());
    }
    return *this;
  }

  virtual ByteBuffer& put(const ByteArray& src, int32_t offset, int32_t length) {
    checkBounds(offset, length, src.size());
    if (length > remaining()) {
      throw std::runtime_error("BufferOverflowException");
    }
    int32_t end = offset + length;
    for (int32_t i = offset; i < end; i++) {
      put(src[i]);
    }
    return *this;
  }

  ByteBuffer& put(const ByteArray& src) { return put(src, 0, src.size()); }

 public:
  bool hasArray() override final { return (byte_array_ != nullptr) && !is_read_only_; }

  char* array() override final {
    if (byte_array_ == nullptr) {
      throw std::runtime_error("UnsupportedOperationException");
    }
    if (is_read_only_) {
      throw std::runtime_error("ReadOnlyBufferException");
    }
    return byte_array_->array();
  }

  int32_t arrayOffset() override final {
    if (byte_array_ == nullptr) {
      throw std::runtime_error("UnsupportedOperationException");
    }
    if (is_read_only_) {
      throw std::runtime_error("ReadOnlyBufferException");
    }
    return offset_;
  }

  virtual ByteBuffer& compact() = 0;

  virtual std::string toString() {
    std::stringstream ss;
    ss << "ByteBuffer"
       << "[pos=" << position() << " lim=" << limit() << " cap=" << capacity() << "]";
    return ss.str();
  }

  inline ByteOrder order() const { return big_endian_ ? ByteOrder::BO_BIG_ENDIAN : ByteOrder::BO_LITTLE_ENDIAN; }

  inline ByteBuffer& order(ByteOrder bo) {
    big_endian_ = (bo == ByteOrder::BO_BIG_ENDIAN);
    return *this;
  }

  inline ByteArrayRef byte_array() { return byte_array_; }

 public:
  virtual int16_t getShort() = 0;
  virtual ByteBuffer& putShort(int16_t value) = 0;
  virtual int16_t getShort(int32_t index) = 0;
  virtual ByteBuffer& putShort(int32_t index, int16_t value) = 0;

  virtual int32_t getInt() = 0;
  virtual ByteBuffer& putInt(int32_t value) = 0;
  virtual int32_t getInt(int32_t index) = 0;
  virtual ByteBuffer& putInt(int32_t index, int32_t value) = 0;

  virtual int64_t getLong() = 0;
  virtual ByteBuffer& putLong(int64_t value) = 0;
  virtual int64_t getLong(int32_t index) = 0;
  virtual ByteBuffer& putLong(int32_t index, int64_t value) = 0;

  virtual float getFloat() = 0;
  virtual ByteBuffer& putFloat(float value) = 0;
  virtual float getFloat(int32_t index) = 0;
  virtual ByteBuffer& putFloat(int32_t index, float value) = 0;

  virtual double getDouble() = 0;
  virtual ByteBuffer& putDouble(double value) = 0;
  virtual double getDouble(int32_t index) = 0;
  virtual ByteBuffer& putDouble(int32_t index, double value) = 0;

 protected:
  ByteArrayRef byte_array_;
  int32_t offset_;
  bool is_read_only_;

  bool big_endian_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_IO_BYTEBUFFER_HPP_