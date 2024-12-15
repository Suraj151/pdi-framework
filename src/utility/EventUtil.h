/****************************** Event service *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EVENT_UTILITY_H_
#define _EVENT_UTILITY_H_

#include <config/EventConfig.h>
#include "iUtilityInterface.h"

/**
 * EventUtil class
 */
class EventUtil
{

public:
  /**
   * EventUtil constructor.
   */
  EventUtil();
  /**
   * EventUtil destructor
   */
  ~EventUtil();

  void begin(iUtilityInterface *_iutil);
  bool add_event_listener(event_name_t _event, CallBackVoidPointerArgFn _handler);
  void execute_event(event_name_t _event, void *_arg = nullptr);

  int m_last_event;
  unsigned long m_last_event_millis;

protected:
  /**
   * @var	iUtilityInterface* m_util
   */
  iUtilityInterface *m_util;

  /**
   * @var	pdiutil::vector<event_listener_t> vector
   */
  pdiutil::vector<event_listener_t> m_event_listeners;
};

extern EventUtil __utl_event;

#endif
