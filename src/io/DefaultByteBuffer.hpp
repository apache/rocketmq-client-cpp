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
#ifndef ROCKETMQ_IO_DEFAULTBYTEBUFFER_HPP_
#define ROCKETMQ_IO_DEFAULTBYTEBUFFER_HPP_

#include <cstdlib>  // std::memcpy

#include <algorithm>  // std::move
#include <typeindex>  // std::type_index

#include "ByteBuffer.hpp"

namespace rocketmq {

class DefaultByteBuffer : public ByteBuffer {
 protected:
  friend class ByteBuffer;

  DefaultByteBuffer(int32_t cap, int32_t lim) : ByteBuffer(-1, 0, lim, cap, std::make_shared<ByteArray>(cap), 0) {}

  DefaultByteBuffer(ByteArrayRef buf, int32_t off, int32_t len) : ByteBuffer(-1, off, off + len, buf->size(), buf, 0) {}

  DefaultByteBuffer(ByteArrayRef buf, int32_t mark, int32_t pos, int32_t lim, int32_t cap, int32_t off)
      : ByteBuffer(mark, pos, lim, cap, std::move(buf), off) {}

 public:
  ByteBuffer* slice() override {
    return new DefaultByteBuffer(byte_array_, -1, 0, remaining(), remaining(), position() + offset_);
  }

  ByteBuffer* duplicate() override {
    return new DefaultByteBuffer(byte_array_, mark_value(), position(), limit(), capacity(), offset_);
  }

  ByteBuffer* asReadOnlyBuffer() override {
    // return new HeapByteBufferR(byte_array_, mark_value(), position(), limit(), capacity(), offset_);
    return nullptr;
  }

  char get() override { return (*byte_array_)[ix(nextGetIndex())]; }
  char get(int i) const override { return (*byte_array_)[ix(checkIndex(i))]; }

  ByteBuffer& get(ByteArray& dst, int32_t offset, int length) override {
    checkBounds(offset, length, dst.size());
    if (length > remaining()) {
      throw std::runtime_error("BufferUnderflowException");
    }
    std::memcpy(dst.array() + offset, byte_array_->array() + ix(position()), length);
    position(position() + length);
    return *this;
  }

  ByteBuffer& put(char x) override {
    (*byte_array_)[ix(nextPutIndex())] = x;
    return *this;
  }

  ByteBuffer& put(int32_t i, char x) override {
    (*byte_array_)[ix(checkIndex(i))] = x;
    return *this;
  }

  ByteBuffer& put(const ByteArray& src, int offset, int length) override {
    checkBounds(offset, length, src.size());
    if (length > remaining()) {
      throw std::runtime_error("BufferOverflowException");
    }
    std::memcpy(byte_array_->array() + ix(position()), src.array() + offset, length);
    position(position() + length);
    return *this;
  }

  ByteBuffer& put(ByteBuffer& src) override {
    if (std::type_index(typeid(src)) == std::type_index(typeid(DefaultByteBuffer))) {
      if (&src == this) {
        throw std::invalid_argument("");
      }
      DefaultByteBuffer& sb = dynamic_cast<DefaultByteBuffer&>(src);
      int32_t n = sb.remaining();
      if (n > remaining()) {
        throw std::runtime_error("BufferOverflowException");
      }
      std::memcpy(byte_array_->array() + ix(position()), sb.byte_array_->array() + sb.ix(sb.position()), n);
      sb.position(sb.position() + n);
      position(position() + n);
    } else {
      ByteBuffer::put(src);
    }
    return *this;
  }

 public:
  bool isReadOnly() override { return false; }

  ByteBuffer& compact() override {
    std::memcpy(byte_array_->array() + ix(0), byte_array_->array() + ix(position()), remaining());
    position(remaining());
    limit(capacity());
    discardMark();
    return *this;
  }

 public:
  int16_t getShort() override {
    return ByteOrderUtil::Read<uint16_t>(byte_array_->array() + ix(nextGetIndex(2)), big_endian_);
  }

  int16_t getShort(int32_t i) override {
    return ByteOrderUtil::Read<uint16_t>(byte_array_->array() + ix(checkIndex(i, 2)), big_endian_);
  }

  ByteBuffer& putShort(int16_t x) override {
    ByteOrderUtil::Write(byte_array_->array() + ix(nextPutIndex(2)), x, big_endian_);
    return *this;
  }

  ByteBuffer& putShort(int32_t i, int16_t x) override {
    ByteOrderUtil::Write(byte_array_->array() + ix(checkIndex(i, 2)), x, big_endian_);
    return *this;
  }

  int32_t getInt() override {
    return ByteOrderUtil::Read<uint32_t>(byte_array_->array() + ix(nextGetIndex(4)), big_endian_);
  }
  int32_t getInt(int32_t i) override {
    return ByteOrderUtil::Read<uint32_t>(byte_array_->array() + ix(checkIndex(i, 4)), big_endian_);
  }

  ByteBuffer& putInt(int32_t x) override {
    ByteOrderUtil::Write(byte_array_->array() + ix(nextPutIndex(4)), x, big_endian_);
    return *this;
  }

  ByteBuffer& putInt(int32_t i, int32_t x) override {
    ByteOrderUtil::Write(byte_array_->array() + ix(checkIndex(i, 4)), x, big_endian_);
    return *this;
  }

  int64_t getLong() override {
    return ByteOrderUtil::Read<uint64_t>(byte_array_->array() + ix(nextGetIndex(8)), big_endian_);
  }

  int64_t getLong(int32_t i) override {
    return ByteOrderUtil::Read<uint64_t>(byte_array_->array() + ix(checkIndex(i, 8)), big_endian_);
  }

  ByteBuffer& putLong(int64_t x) override {
    ByteOrderUtil::Write(byte_array_->array() + ix(nextPutIndex(8)), x, big_endian_);
    return *this;
  }

  ByteBuffer& putLong(int32_t i, int64_t x) override {
    ByteOrderUtil::Write(byte_array_->array() + ix(checkIndex(i, 8)), x, big_endian_);
    return *this;
  }

  float getFloat() override {
    return ByteOrderUtil::ReinterpretRawBits<float, uint32_t>(
        ByteOrderUtil::Read<uint32_t>(byte_array_->array() + ix(nextGetIndex(4)), big_endian_));
  }

  float getFloat(int32_t i) override {
    return ByteOrderUtil::ReinterpretRawBits<float, uint32_t>(
        ByteOrderUtil::Read<uint32_t>(byte_array_->array() + ix(checkIndex(i, 4)), big_endian_));
  }

  ByteBuffer& putFloat(float x) override {
    ByteOrderUtil::Write(byte_array_->array() + ix(nextPutIndex(4)),
                         ByteOrderUtil::ReinterpretRawBits<uint32_t, float>(x), big_endian_);
    return *this;
  }

  ByteBuffer& putFloat(int32_t i, float x) override {
    ByteOrderUtil::Write(byte_array_->array() + ix(checkIndex(i, 4)),
                         ByteOrderUtil::ReinterpretRawBits<uint32_t, float>(x), big_endian_);
    return *this;
  }

  double getDouble() override {
    return ByteOrderUtil::ReinterpretRawBits<double, uint64_t>(
        ByteOrderUtil::Read<uint64_t>(byte_array_->array() + ix(nextGetIndex(8)), big_endian_));
  }

  double getDouble(int32_t i) override {
    return ByteOrderUtil::ReinterpretRawBits<double, uint64_t>(
        ByteOrderUtil::Read<uint64_t>(byte_array_->array() + ix(checkIndex(i, 8)), big_endian_));
  }

  ByteBuffer& putDouble(double x) override {
    ByteOrderUtil::Write(byte_array_->array() + ix(nextPutIndex(8)),
                         ByteOrderUtil::ReinterpretRawBits<uint64_t, double>(x), big_endian_);
    return *this;
  }

  ByteBuffer& putDouble(int32_t i, double x) override {
    ByteOrderUtil::Write(byte_array_->array() + ix(checkIndex(i, 8)),
                         ByteOrderUtil::ReinterpretRawBits<uint64_t, double>(x), big_endian_);
    return *this;
  }

 protected:
  inline int32_t ix(int32_t i) const { return i + offset_; }
};

}  // namespace rocketmq

#endif  // ROCKETMQ_IO_DEFAULTBYTEBUFFER_HPP_
