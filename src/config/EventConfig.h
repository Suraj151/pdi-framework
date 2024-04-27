/*************************** Event Config page ********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef _EVENT_CONFIG_H_
#define _EVENT_CONFIG_H_

#include "Common.h"

/**
 * max event listener callbacks
 */
#define MAX_EVENT_LISTENERS	MAX_SCHEDULABLE_TASKS

/**
* available event names
*/
typedef enum event_name{

  EVENT_WIFI_STA_CONNECTED = 0,
  EVENT_WIFI_STA_DISCONNECTED,
  EVENT_WIFI_AP_STACONNECTED,
  EVENT_WIFI_AP_STADISCONNECTED,
  EVENT_FACTORY_RESET,
  EVENT_NAME_MAX,
} event_name_t;

/**
* event listener struct type for event
*/
typedef struct event_listener {
  
  // Default Constructor
  event_listener(){
    clear();
  }

  // Clear members method
  void clear(){
    _event = 0;
    _event_handler = nullptr;
  }

  int _event;
  CallBackVoidPointerArgFn _event_handler;
} event_listener_t;

#endif
