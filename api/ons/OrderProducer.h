#pragma once

#include "Message.h"
#include "SendResultONS.h"

ONS_NAMESPACE_BEGIN

class ONSCLIENT_API OrderProducer {
public:
  virtual ~OrderProducer() = default;

  /**
   * @brief Start the instance, allowing it to allocate preparing resources, start internal threads to orchastrate
   * internal components.
   *
   */
  virtual void start() = 0;

  /**
   * @brief Shutdown the instance, which release all internally allocated resources.
   *
   */
  virtual void shutdown() = 0;

  /**
   * @brief Send message pertaining to the specified message_group. Messages of the same message_group go to the same
   * message queue.
   *
   * If errors encountered, ONSClientException would be raised.
   *
   * @param message
   * @param message_group
   * @return SendResultONS
   */
  virtual SendResultONS send(Message& message, std::string message_group) noexcept(false) = 0;
};

ONS_NAMESPACE_END