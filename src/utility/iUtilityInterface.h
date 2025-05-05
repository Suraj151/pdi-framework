/****************************** Utility Interface *****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The iUtilityInterface class defines an abstract interface for utility functions
that must be implemented by the target device. It provides a standardized API
for operations such as logging, timing, task management, and yielding control.

Author          : Suraj I.
Created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_DEVICE_UTILITY_INTERFACE_H_
#define _I_DEVICE_UTILITY_INTERFACE_H_

#include "DataTypeDef.h"
#include "iIOInterface.h"

/**
 * @class iUtilityInterface
 * @brief Abstract interface for utility functions in the PDI stack.
 *
 * The iUtilityInterface class provides a set of pure virtual methods that must
 * be implemented by the target device to satisfy the requirements of the PDI stack.
 * These methods include logging, timing, task management, and yielding control.
 *
 * Features:
 * - Provides a standardized API for utility operations.
 * - Abstracts device-specific implementations of utility functions.
 * - Supports logging, timing
 *
 */
class iUtilityInterface
{

public:
  /**
   * @brief Destructor for the iUtilityInterface class.
   *
   * Ensures proper cleanup of resources when the interface is destroyed.
   */
  virtual ~iUtilityInterface() {}

  /**
   * @brief Waits for a specified timeout in milliseconds.
   * @param timeoutms The timeout duration in milliseconds.
   */
  virtual void wait(uint64_t timeoutms) = 0;

  /**
   * @brief Gets the current time in milliseconds since the system started.
   * @return The current time in milliseconds.
   */
  virtual uint32_t millis_now() = 0;

  /**
   * @brief Logs a message with a specified log type.
   * @param log_type The type of log (e.g., INFO, ERROR, DEBUG).
   * @param content The message content to log.
   */
  virtual void log(logger_type_t log_type, const char *content) = 0;

  /**
   * @brief Prints the list of tasks to a terminal interface.
   * @param tasks A vector of tasks to print.
   * @param terminal The terminal interface to print the tasks to.
   */
  virtual void printtasks(pdiutil::vector<task_t> &tasks, iTerminalInterface *terminal) = 0;

  /**
   * @brief Yields control to allow other tasks or processes to execute.
   */
  virtual void yield() = 0;

  /**
   * @brief Check and start if possible of measure stack in use for next functions
   */
  virtual bool can_measure_stack(){ return false; }

  /**
   * @brief Get used stack size of last function. Must call can_measure_stack once 
   * before using this function
   */
  virtual int64_t measure_lastfn_stack(){ return -1; }
};

#endif
