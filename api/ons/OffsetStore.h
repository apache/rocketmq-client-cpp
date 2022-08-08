#pragma once

#include <cstdint>
#include <string>

#include "ONSClient.h"

ONS_NAMESPACE_BEGIN

/**
 * @brief By default, consumers of broadcasting model skip existing records when at startup. Application developers may
 * make use of this class to pick up from the previous progress.
 */
class OffsetStore {
public:
  /**
   * @brief Read offset of the given queue from persist source.
   *
   * @param queue
   * @param offset
   * @return true it manages to read offset from persistence layer;
   * @return false if something wrong occurred;
   */
  virtual bool readOffset(const std::string& queue, std::int64_t& offset) = 0;


  /**
   * @brief Save offset of the given queue;
   * 
   * @param queue 
   * @param offset 
   */
  virtual void writeOffset(const std::string& queue, std::int64_t offset) = 0;
};

ONS_NAMESPACE_END