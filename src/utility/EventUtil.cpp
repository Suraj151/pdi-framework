/****************************** Event Service *********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#include "EventUtil.h"

/**
 * @brief Constructor for the EventUtil class.
 *
 * Initializes the EventUtil object and sets default values for internal state.
 */
EventUtil::EventUtil() : m_last_event(0),
                         m_last_event_millis(0),
                         m_util(nullptr)
{
}

/**
 * @brief Destructor for the EventUtil class.
 *
 * Cleans up resources used by the EventUtil object.
 */
EventUtil::~EventUtil()
{
}

/**
 * @brief Initializes the EventUtil object with a utility interface.
 *
 * This method sets the utility interface to be used for event handling.
 *
 * @param _iutil Pointer to an iUtilityInterface implementation.
 */
void EventUtil::begin(iUtilityInterface *_iutil)
{
  m_util = _iutil;
}

/**
 * @brief Adds an event listener for a specific event.
 *
 * Registers a callback function to be executed when the specified event is triggered.
 * Returns true if the listener is successfully added, false otherwise.
 *
 * @param _event The name of the event to listen for.
 * @param _handler The callback function to execute when the event is triggered.
 * @return True if the listener was added successfully, false otherwise.
 */
bool EventUtil::add_event_listener(event_name_t _event, CallBackVoidPointerArgFn _handler)
{
  if (this->m_event_listeners.size() < MAX_EVENT_LISTENERS)
  {
    event_listener_t _new_event;
    _new_event._event = _event;
    _new_event._event_handler = _handler;
    this->m_event_listeners.push_back(_new_event);
    return true;
  }
  return false;
}

/**
 * @brief Executes all listeners for a specific event.
 *
 * Triggers the callback functions associated with the specified event and passes
 * an optional argument to the handlers.
 *
 * @param _event The name of the event to execute.
 * @param _arg Optional argument to pass to the event handlers.
 */
void EventUtil::execute_event(event_name_t _event, void *_arg)
{
  if (nullptr != m_util)
  {
    char content[25];
    memset(content, 0, 25);
    sprintf(content, "\ne:%d\n", (int)_event);
    m_util->log(INFO_LOG, content);
  }

  for (uint16_t i = 0; i < this->m_event_listeners.size(); i++)
  {
    if (this->m_event_listeners[i]._event == _event && this->m_event_listeners[i]._event_handler)
    {
      this->m_event_listeners[i]._event_handler(_arg);
    }
  }
}

/**
 * @brief Global instance of the EventUtil class.
 *
 * This instance is used throughout the PDI stack for managing and executing events.
 */
EventUtil __utl_event;
