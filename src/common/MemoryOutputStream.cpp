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
#include "MemoryOutputStream.h"

#include <algorithm>
#include <cstring>

namespace rocketmq {

MemoryOutputStream::MemoryOutputStream(const size_t initialSize)
    : poolToUse(&internalPool), externalData(nullptr), position(0), size(0), availableSize(0) {
  internalPool.setSize(initialSize, false);
}

MemoryOutputStream::MemoryOutputStream(MemoryPool& memoryBlockToWriteTo, const bool appendToExistingBlockContent)
    : poolToUse(&memoryBlockToWriteTo), externalData(NULL), position(0), size(0), availableSize(0) {
  if (appendToExistingBlockContent) {
    position = size = memoryBlockToWriteTo.getSize();
  }
}

MemoryOutputStream::MemoryOutputStream(void* destBuffer, size_t destBufferSize)
    : poolToUse(NULL), externalData(destBuffer), position(0), size(0), availableSize(destBufferSize) {}

MemoryOutputStream::~MemoryOutputStream() {
  trimExternalBlockSize();
}

void MemoryOutputStream::flush() {
  trimExternalBlockSize();
}

void MemoryOutputStream::trimExternalBlockSize() {
  if (poolToUse != &internalPool && poolToUse != NULL) {
    poolToUse->setSize(size, false);
  }
}

void MemoryOutputStream::preallocate(const size_t bytesToPreallocate) {
  if (poolToUse != NULL) {
    poolToUse->ensureSize(bytesToPreallocate + 1);
  }
}

void MemoryOutputStream::reset() {
  position = 0;
  size = 0;
}

char* MemoryOutputStream::prepareToWrite(size_t numBytes) {
  size_t storageNeeded = position + numBytes;

  char* data;

  if (poolToUse != NULL) {
    if (storageNeeded >= (unsigned int)(poolToUse->getSize())) {
      poolToUse->ensureSize((storageNeeded + std::min(storageNeeded / 2, (size_t)(1024 * 1024)) + 32) & ~31u);
    }

    data = static_cast<char*>(poolToUse->getData());
  } else {
    if (storageNeeded > availableSize) {
      return NULL;
    }

    data = static_cast<char*>(externalData);
  }

  char* const writePointer = data + position;
  position += numBytes;
  size = std::max(size, position);
  return writePointer;
}

bool MemoryOutputStream::write(const void* const buffer, size_t howMany) {
  if (howMany == 0) {
    return true;
  }

  if (char* dest = prepareToWrite(howMany)) {
    memcpy(dest, buffer, howMany);
    return true;
  }

  return false;
}

bool MemoryOutputStream::writeRepeatedByte(uint8_t byte, size_t howMany) {
  if (howMany == 0) {
    return true;
  }

  if (char* dest = prepareToWrite(howMany)) {
    memset(dest, byte, howMany);
    return true;
  }

  return false;
}

MemoryPool MemoryOutputStream::getMemoryBlock() const {
  return MemoryPool(getData(), getDataSize());
}

const void* MemoryOutputStream::getData() const {
  if (poolToUse == NULL) {
    return externalData;
  }

  if (poolToUse->getSize() > size) {
    poolToUse->getData()[size] = 0;
  }

  return poolToUse->getData();
}

bool MemoryOutputStream::setPosition(int64_t newPosition) {
  if (newPosition <= (int64_t)size) {
    // ok to seek backwards
    if (newPosition < 0) {
      position = 0;
    } else {
      position = (int64_t)size < newPosition ? size : newPosition;
    }
    return true;
  }

  // can't move beyond the end of the stream..
  return false;
}

int64_t MemoryOutputStream::writeFromInputStream(InputStream& source, int64_t maxNumBytesToWrite) {
  // before writing from an input, see if we can preallocate to make it more efficient..
  int64_t availableData = source.getTotalLength() - source.getPosition();

  if (availableData > 0) {
    if (maxNumBytesToWrite > availableData || maxNumBytesToWrite < 0) {
      maxNumBytesToWrite = availableData;
    }

    if (poolToUse != NULL) {
      preallocate(poolToUse->getSize() + (size_t)maxNumBytesToWrite);
    }
  }

  return OutputStream::writeFromInputStream(source, maxNumBytesToWrite);
}

OutputStream& operator<<(OutputStream& stream, const MemoryOutputStream& streamToRead) {
  const size_t dataSize = streamToRead.getDataSize();

  if (dataSize > 0) {
    stream.write(streamToRead.getData(), dataSize);
  }

  return stream;
}

}  // namespace rocketmq
