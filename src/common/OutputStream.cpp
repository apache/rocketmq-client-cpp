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
#include "OutputStream.h"

#include <algorithm>
#include <limits>

#include "big_endian.h"

namespace rocketmq {
//==============================================================================
OutputStream::OutputStream() {}

OutputStream::~OutputStream() {}

//==============================================================================
bool OutputStream::writeBool(const bool b) {
  return writeByte(b ? (char)1 : (char)0);
}

bool OutputStream::writeByte(char byte) {
  return write(&byte, 1);
}

bool OutputStream::writeRepeatedByte(uint8_t byte, size_t numTimesToRepeat) {
  for (size_t i = 0; i < numTimesToRepeat; ++i)
    if (!writeByte((char)byte))
      return false;

  return true;
}

bool OutputStream::writeShortBigEndian(short value) {
  unsigned short v;
  char pShort[sizeof(v)];
  WriteBigEndian(pShort, (unsigned short)value);
  return write(pShort, 2);
}

bool OutputStream::writeIntBigEndian(int value) {
  unsigned int v;
  char pInt[sizeof(v)];
  WriteBigEndian(pInt, (unsigned int)value);
  return write(pInt, 4);
}

bool OutputStream::writeInt64BigEndian(int64_t value) {
  uint64_t v;
  char pUint64[sizeof(v)];
  WriteBigEndian(pUint64, (uint64_t)value);
  return write(pUint64, 8);
}

bool OutputStream::writeFloatBigEndian(float value) {
  union {
    int asInt;
    float asFloat;
  } n;
  n.asFloat = value;
  return writeIntBigEndian(n.asInt);
}

bool OutputStream::writeDoubleBigEndian(double value) {
  union {
    int64_t asInt;
    double asDouble;
  } n;
  n.asDouble = value;
  return writeInt64BigEndian(n.asInt);
}

int64_t OutputStream::writeFromInputStream(InputStream& source, int64_t numBytesToWrite) {
  if (numBytesToWrite < 0)
    numBytesToWrite = std::numeric_limits<int64_t>::max();

  int64_t numWritten = 0;

  while (numBytesToWrite > 0) {
    char buffer[8192];
    const int num = source.read(buffer, (int)std::min(numBytesToWrite, (int64_t)sizeof(buffer)));

    if (num <= 0)
      break;

    write(buffer, (size_t)num);

    numBytesToWrite -= num;
    numWritten += num;
  }

  return numWritten;
}
}  // namespace rocketmq
