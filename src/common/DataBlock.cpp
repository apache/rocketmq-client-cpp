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
#include "DataBlock.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace rocketmq {

//######################################
// MemoryBlock
//######################################

void MemoryBlock::copyTo(void* dst, ssize_t offset, size_t num) const {
  char* d = static_cast<char*>(dst);

  if (offset < 0) {
    memset(d, 0, -offset);
    d += (-offset);
    num -= (-offset);
    offset = 0;
  }

  if (offset + num > size_) {
    size_t newNum = size_ - offset;
    memset(d + newNum, 0, num - newNum);
    num = newNum;
  }

  if (num > 0) {
    memcpy(d, data_ + offset, num);
  }
}

//######################################
// MemoryPool
//######################################

MemoryPool::MemoryPool() : MemoryBlock() {}

MemoryPool::MemoryPool(size_t initialSize, bool initialiseToZero) : MemoryBlock(nullptr, initialSize) {
  if (size_ > 0) {
    data_ = static_cast<char*>(initialiseToZero ? std::calloc(initialSize, sizeof(char))
                                                : std::malloc(initialSize * sizeof(char)));
  }
}

MemoryPool::MemoryPool(const void* const dataToInitialiseFrom, size_t sizeInBytes) : MemoryBlock(nullptr, sizeInBytes) {
  if (size_ > 0) {
    data_ = static_cast<char*>(std::malloc(size_ * sizeof(char)));

    if (dataToInitialiseFrom != nullptr) {
      memcpy(data_, dataToInitialiseFrom, size_);
    }
  }
}

MemoryPool::MemoryPool(const MemoryPool& other) : MemoryBlock(nullptr, other.size_) {
  if (size_ > 0) {
    data_ = static_cast<char*>(std::malloc(size_ * sizeof(char)));
    memcpy(data_, other.data_, size_);
  }
}

MemoryPool::MemoryPool(MemoryPool&& other) : MemoryBlock(other.data_, other.size_) {
  other.size_ = 0;
  other.data_ = nullptr;
}

MemoryPool::~MemoryPool() {
  std::free(data_);
}

MemoryPool& MemoryPool::operator=(const MemoryPool& other) {
  if (this != &other) {
    setSize(other.size_, false);
    memcpy(data_, other.data_, size_);
  }
  return *this;
}

MemoryPool& MemoryPool::operator=(MemoryPool&& other) {
  if (this != &other) {
    std::free(data_);

    size_ = other.size_;
    data_ = other.data_;

    other.size_ = 0;
    other.data_ = nullptr;
  }
  return *this;
}

//==============================================================================
bool MemoryPool::matches(const void* dataToCompare, size_t dataSize) const {
  return size_ == dataSize && memcmp(data_, dataToCompare, size_) == 0;
}

bool MemoryPool::operator==(const MemoryPool& other) const {
  return matches(other.data_, other.size_);
}

bool MemoryPool::operator!=(const MemoryPool& other) const {
  return !operator==(other);
}

//==============================================================================
void MemoryPool::reset() {
  reset(nullptr, 0);
}

void MemoryPool::reset(char* data, size_t size) {
  if (size != 0) {
    throw std::runtime_error("MemoryBlock can't set external pointer as data.");
  }

  std::free(data_);
  data_ = nullptr;
  size_ = 0;
}

// this will resize the block to this size_
void MemoryPool::setSize(size_t newSize, bool initialiseToZero) {
  if (size_ != newSize) {
    if (newSize <= 0) {
      reset();
    } else {
      if (data_ != nullptr) {
        data_ = static_cast<char*>(std::realloc(data_, newSize * sizeof(char)));

        if (initialiseToZero && (newSize > size_)) {
          memset(data_ + size_, 0, newSize - size_);
        }
      } else {
        data_ = static_cast<char*>(initialiseToZero ? std::calloc(newSize, sizeof(char))
                                                    : std::malloc(newSize * sizeof(char)));
      }

      size_ = newSize;
    }
  }
}

void MemoryPool::ensureSize(size_t minimumSize, bool initialiseToZero) {
  if (size_ < minimumSize) {
    setSize(minimumSize, initialiseToZero);
  }
}

//==============================================================================
void MemoryPool::fillWith(int value) {
  memset(data_, value, size_);
}

void MemoryPool::append(const void* const srcData, size_t numBytes) {
  if (numBytes > 0) {
    size_t oldSize = size_;
    setSize(size_ + numBytes);
    memcpy(data_ + oldSize, srcData, numBytes);
  }
}

void MemoryPool::replaceWith(const void* const srcData, size_t numBytes) {
  if (numBytes > 0) {
    setSize(numBytes);
    memcpy(data_, srcData, numBytes);
  }
}

void MemoryPool::insert(const void* const srcData, size_t numBytes, size_t insertPosition) {
  if (numBytes > 0) {
    insertPosition = std::min(insertPosition, size_);
    size_t trailingDataSize = size_ - insertPosition;
    setSize(size_ + numBytes, false);

    if (trailingDataSize > 0) {
      memmove(data_ + insertPosition + numBytes, data_ + insertPosition, trailingDataSize);
    }

    memcpy(data_ + insertPosition, srcData, numBytes);
  }
}

void MemoryPool::removeSection(size_t startByte, size_t numBytesToRemove) {
  if (startByte + numBytesToRemove >= size_) {
    setSize(startByte);
  } else if (numBytesToRemove > 0) {
    memmove(data_ + startByte, data_ + startByte + numBytesToRemove, size_ - (startByte + numBytesToRemove));
    setSize(size_ - numBytesToRemove);
  }
}

void MemoryPool::copyFrom(const void* src, ssize_t offset, size_t num) {
  const char* d = static_cast<const char*>(src);

  if (offset < 0) {
    d += (-offset);
    num -= (-offset);
    offset = 0;
  }

  if (offset + num > size_) {
    num = size_ - offset;
  }

  if (num > 0) {
    memcpy(data_ + offset, d, num);
  }
}

//######################################
// MemoryView
//######################################

void MemoryView::reset() {
  reset(nullptr, 0);
}

void MemoryView::reset(char* data, size_t size) {
  throw std::runtime_error("MemoryView can't reset().");
}

}  // namespace rocketmq
