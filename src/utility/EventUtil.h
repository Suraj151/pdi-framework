/****************************** Event Utility *********************************
This file is part of the PDI stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The EventUtil class provides a mechanism for managing and executing events
within the PDI stack. It allows components to register event listeners and
trigger events with optional arguments. This utility is designed to work
with the iUtilityInterface, which provides the underlying utility functions
required for event handling.


Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _EVENT_UTILITY_H_
#define _EVENT_UTILITY_H_

#include <config/EventConfig.h>
#include "iUtilityInterface.h"

/**
 * @class EventUtil
 * @brief Utility class for managing and executing events.
 *
 * The EventUtil class provides a centralized mechanism for handling events
 * within the PDI stack. It allows components to register event listeners
 * and trigger events with optional arguments. This utility is designed to
 * work with the iUtilityInterface, which provides the underlying utility
 * functions required for event handling.
 *
 * Features:
 * - Register event listeners with specific event names.
 * - Trigger events and execute associated callbacks.
 * - Maintain a history of the last event and its timestamp.
 *
 */
class EventUtil
{

public:
  /**
   * @brief Constructor for the EventUtil class.
   *
   * Initializes the EventUtil object and sets up the internal state.
   */
  EventUtil();

  /**
   * @brief Destructor for the EventUtil class.
   *
   * Cleans up resources used by the EventUtil object.
   */
  ~EventUtil();

  /**
   * @brief Initializes the EventUtil object with a utility interface.
   * @param _iutil Pointer to an iUtilityInterface implementation.
   */
  void begin(iUtilityInterface *_iutil);

  /**
   * @brief Adds an event listener for a specific event.
   * @param _event The name of the event to listen for.
   * @param _handler The callback function to execute when the event is triggered.
   * @return True if the listener was added successfully, false otherwise.
   */
  bool add_event_listener(event_name_t _event, CallBackVoidPointerArgFn _handler);

  /**
   * @brief Executes an event and calls the associated listeners.
   * @param _event The name of the event to execute.
   * @param _arg Optional argument to pass to the event handler.
   */
  void execute_event(event_name_t _event, void *_arg = nullptr);

  /**
   * @var m_last_event
   * @brief Stores the name of the last executed event.
   */
  int m_last_event;

  /**
   * @var m_last_event_millis
   * @brief Stores the timestamp of the last executed event in milliseconds.
   */
  unsigned long m_last_event_millis;

protected:
  /**
   * @var iUtilityInterface* m_util
   * @brief Pointer to the utility interface used for event handling.
   */
  iUtilityInterface *m_util;

  /**
   * @var pdiutil::vector<event_listener_t> m_event_listeners
   * @brief Vector of registered event listeners.
   */
  pdiutil::vector<event_listener_t> m_event_listeners;
};

/**
 * @brief Global instance of the EventUtil class.
 *
 * This instance is used throughout the PDI stack for managing and executing events.
 */
extern EventUtil __utl_event;

#endif
