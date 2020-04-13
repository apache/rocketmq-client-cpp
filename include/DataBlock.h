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
#ifndef __DATA_BLOCK_H__
#define __DATA_BLOCK_H__

#include <memory>

#include "RocketMQClient.h"

namespace rocketmq {

class MemoryBlock;
typedef std::unique_ptr<MemoryBlock> MemoryBlockPtr;
typedef std::shared_ptr<MemoryBlock> MemoryBlockPtr2;

class ROCKETMQCLIENT_API MemoryBlock {
 public:
  MemoryBlock() : MemoryBlock(nullptr, 0) {}
  MemoryBlock(char* data, size_t size) : data_(data), size_(size) {}
  virtual ~MemoryBlock() = default;

  /** Returns a void pointer to the data.
   *
   *  Note that the pointer returned will probably become invalid when the block is resized.
   */
  char* getData() { return data_; }

  const char* getData() const { return data_; }

  /** Returns the block's current allocated size, in bytes. */
  size_t getSize() const { return size_; }

  /** Returns a byte from the memory block.
   *  This returns a reference, so you can also use it to set a byte.
   */
  template <typename Type>
  char& operator[](const Type offset) {
    return data_[offset];
  }

  template <typename Type>
  const char& operator[](const Type offset) const {
    return data_[offset];
  }

  /** Frees all the blocks data, setting its size to 0. */
  virtual void reset() { reset(nullptr, 0); }

  virtual void reset(char* data, size_t size) {
    data_ = data;
    size_ = size;
  }

  /** Copies data from this MemoryBlock to a memory address.
   *
   *  @param destData         the memory location to write to
   *  @param sourceOffset     the offset within this block from which the copied data will be read
   *  @param numBytes         how much to copy (if this extends beyond the limits of the memory block,
   *                          zeros will be used for that portion of the data)
   */
  void copyTo(void* destData, ssize_t sourceOffset, size_t numBytes) const;

 protected:
  char* data_;
  size_t size_;
};

class ROCKETMQCLIENT_API MemoryPool : public MemoryBlock {
 public:
  /** Create an uninitialised block with 0 size. */
  MemoryPool();

  /** Creates a memory block with a given initial size.
   *
   *  @param initialSize          the size of block to create
   *  @param initialiseToZero     whether to clear the memory or just leave it uninitialised
   */
  MemoryPool(size_t initialSize, bool initialiseToZero = false);

  /** Creates a memory block using a copy of a block of data.
   *
   *  @param dataToInitialiseFrom     some data to copy into this block
   *  @param sizeInBytes              how much space to use
   */
  MemoryPool(const void* dataToInitialiseFrom, size_t sizeInBytes);

  /** Creates a copy of another memory block. */
  MemoryPool(const MemoryPool&);

  MemoryPool(MemoryPool&&);

  /** Destructor. */
  ~MemoryPool();

  /** Copies another memory block onto this one.
   *  This block will be resized and copied to exactly match the other one.
   */
  MemoryPool& operator=(const MemoryPool&);

  MemoryPool& operator=(MemoryPool&&);

  /** Returns true if the data in this MemoryBlock matches the raw bytes passed-in. */
  bool matches(const void* data, size_t dataSize) const;

  /** Compares two memory blocks.
   *  @returns true only if the two blocks are the same size and have identical contents.
   */
  bool operator==(const MemoryPool& other) const;

  /** Compares two memory blocks.
   *  @returns true if the two blocks are different sizes or have different contents.
   */
  bool operator!=(const MemoryPool& other) const;

  void reset() override;
  void reset(char* data, size_t size) override;

  /** Resizes the memory block.
   *
   *  Any data that is present in both the old and new sizes will be retained. When enlarging the block, the new
   *  space that is allocated at the end can either be cleared, or left uninitialised.
   *
   *  @param newSize                      the new desired size for the block
   *  @param initialiseNewSpaceToZero     if the block gets enlarged, this determines  whether to clear the new
   *                                      section or just leave it uninitialised
   *  @see ensureSize
   */
  void setSize(size_t newSize, bool initialiseNewSpaceToZero = false);

  /** Increases the block's size only if it's smaller than a given size.
   *
   *  @param minimumSize                  if the block is already bigger than this size, no action will be taken;
   *                                      otherwise it will be increased to this size
   *  @param initialiseNewSpaceToZero     if the block gets enlarged, this determines whether to clear the new section
   *                                      or just leave it uninitialised
   *  @see setSize
   */
  void ensureSize(size_t minimumSize, bool initialiseNewSpaceToZero = false);

  /** Fills the entire memory block with a repeated byte value.
   *  This is handy for clearing a block of memory to zero.
   */
  void fillWith(int valueToUse);

  /** Adds another block of data to the end of this one.
   *  The data pointer must not be null. This block's size will be increasedaccordingly.
   */
  void append(const void* data, size_t numBytes);

  /** Resizes this block to the given size and fills its contents from the supplied buffer.
   *  The data pointer must not be null.
   */
  void replaceWith(const void* data, size_t numBytes);

  /** Inserts some data into the block.
   *  The dataToInsert pointer must not be null. This block's size will be increased accordingly.
   *  If the insert position lies outside the valid range of the block, it will be clipped to within the range before
   *  being used.
   */
  void insert(const void* dataToInsert, size_t numBytesToInsert, size_t insertPosition);

  /** Chops out a section  of the block.
   *
   *  This will remove a section of the memory block and close the gap around it, shifting any subsequent data
   *  downwards and reducing the size of the block.
   *
   *  If the range specified goes beyond the size of the block, it will be clipped.
   */
  void removeSection(size_t startByte, size_t numBytesToRemove);

  /** Copies data into this MemoryBlock from a memory address.
   *
   *  @param srcData              the memory location of the data to copy into this block
   *  @param destinationOffset    the offset in this block at which the data being copied should begin
   *  @param numBytes             how much to copy in (if this goes beyond the size of the memory block,
   *                              it will be clipped so not to do anything nasty)
   */
  void copyFrom(const void* srcData, ssize_t destinationOffset, size_t numBytes);
};

class ROCKETMQCLIENT_API MemoryView : public MemoryBlock {
 public:
  MemoryView(MemoryBlockPtr2 origin, size_t offset) : MemoryView(origin, offset, origin->getSize() - offset) {}
  MemoryView(MemoryBlockPtr2 origin, size_t offset, size_t size)
      : MemoryBlock(origin->getData() + offset, size), origin_(origin) {}

  void reset() override;
  void reset(char* data, size_t size) override;

 private:
  MemoryBlockPtr2 origin_;
};

}  // namespace rocketmq

#endif  // __DATA_BLOCK_H__
