/****************************** Serial Services *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_SERIAL_SERVICE)

#include "SerialServiceProvider.h"
#ifdef ENABLE_CMD_SERVICE
#include <service_provider/cmd/CommandLineServiceProvider.h>
#endif

/**
 * SerialServiceProvider constructor
 */
SerialServiceProvider::SerialServiceProvider() : ServiceProvider(SERVICE_SERIAL, RODT_ATTR("Serial"))
{
}

/**
 * SerialServiceProvider destructor
 */
SerialServiceProvider::~SerialServiceProvider()
{
}

/**
 * initialize the serial service
 *
 * @return bool status
 */
bool SerialServiceProvider::initService(void *arg)
{
  __utl_event.add_event_listener(EVENT_SERIAL_AVAILABLE, [&](void *e){

    if( nullptr != e ){

      serial_event_t *serialevent = reinterpret_cast<serial_event_t *>(e);

      // on receive serial event process the serial data
      this->processSerial(serialevent);
      // Process in async task
      // if( SERIAL_TYPE_UART == serialevent->type ){

      //   __task_scheduler.setTimeout( [&]() { 
      //   }, 1, __i_dvc_ctrl.millis_now() );
      // }
    }
  });

  return ServiceProvider::initService(arg);
}

/**
 * handle processing of received serial data based on its type
 *
 * @param serial_event_t* se
 */
void SerialServiceProvider::processSerial(serial_event_t *se)
{
  if(nullptr != se && nullptr != se->serial){

    se->serial->set_terminal_type(TERMINAL_TYPE_SERIAL);

    if( SERIAL_TYPE_UART == se->type ){

      // process and execute if command has provided
      #ifdef ENABLE_CMD_SERVICE
      iTerminalInterface *terminal = __cmd_service.getTerminal();
      if( nullptr == terminal ){
        __cmd_service.useTerminal(se->serial);
        terminal = __cmd_service.getTerminal();
      }

      // Currently supporting multiple client (tcp, telnet, serial) for command service.
      // And at a time currently only one terminal can be used for command service.
      // So checking the terminal type before proceeding the command service
      // Other client types can override running serial terminal and use it for command service
      if( terminal->get_terminal_type() == TERMINAL_TYPE_SERIAL ){
          __cmd_service.processTerminalInput(se->serial);
      }
      #endif
    }

    // make sure to flush serial if no more data available
    __i_dvc_ctrl.wait(1);
    //if(!se->serial->available()) 
    se->serial->flush();
  }
}

SerialServiceProvider __serial_service;

#endif