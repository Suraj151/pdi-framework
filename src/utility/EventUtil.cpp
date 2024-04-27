/****************************** Event service *********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "EventUtil.h"

/**
 * EventUtil constructor.
 */
EventUtil::EventUtil() : m_last_event(0),
                         m_last_event_millis(0),
                         m_util(nullptr)
{
}

/**
 * EventUtil destructor
 */
EventUtil::~EventUtil()
{
}

/**
 * begin event service. set wifi event handler
 *
 */
void EventUtil::begin(iUtilityInterface *_iutil)
{
  m_util = _iutil;
}

/**
 * add event listener. added listener will be executed on given event.
 * it return true on successfull addition of listener
 *
 * @param		event_name_t	_event
 * @param		CallBackVoidPointerArgFn	_handler
 * @return  bool
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
 * here we will execute perticular event listeners with their accepted argument
 *
 * @param		event_name_t	_event
 * @param		void*|nullptr	_arg
 */
void EventUtil::execute_event(event_name_t _event, void *_arg)
{
  if( nullptr != m_util )
  {
    char content[25];
    memset(content, 0, 25);
    sprintf(content, "\nExecuting event : %d\n", (int)_event);
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

EventUtil __utl_event;
