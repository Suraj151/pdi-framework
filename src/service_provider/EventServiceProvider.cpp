/****************************** Event service *********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "EventServiceProvider.h"

/**
 * EventServiceProvider constructor.
 */
EventServiceProvider::EventServiceProvider():
  m_last_event(0),
  m_last_event_millis(0)
{
}

/**
 * EventServiceProvider destructor
 */
EventServiceProvider::~EventServiceProvider(){
}

/**
 * begin event service. set wifi event handler
 *
 */
void EventServiceProvider::begin(){

  wifi_set_event_handler_cb( (wifi_event_handler_cb_t)&EventServiceProvider::wifi_event_handler_cb );
}

/**
 * add event listener. added listener will be executed on given event.
 * it return true on successfull addition of listener
 *
 * @param		event_name_t	_event
 * @param		CallBackVoidPointerArgFn	_handler
 * @return  bool
 */
bool EventServiceProvider::add_event_listener( event_name_t _event, CallBackVoidPointerArgFn _handler ){

  if( this->m_event_listeners.size() < MAX_EVENT_LISTENERS ){

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
void EventServiceProvider::execute_event( event_name_t _event, void* _arg ){

  for ( uint16_t i = 0; i < this->m_event_listeners.size(); i++) {

    if( this->m_event_listeners[i]._event == _event && this->m_event_listeners[i]._event_handler ){

      LogFmtI("\nExecuting event : %d\n", (int)_event);
      this->m_event_listeners[i]._event_handler(_arg);
    }
  }
}

/**
 * static wifi event handler
 *
 */
void EventServiceProvider::wifi_event_handler_cb( System_Event_t * _event ){

  __event_service.execute_event( (event_name_t)_event->event, (void*)&_event->event_info );
}

EventServiceProvider __event_service;
