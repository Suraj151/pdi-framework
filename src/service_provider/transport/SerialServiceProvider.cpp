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
SerialServiceProvider::SerialServiceProvider() : ServiceProvider(SERVICE_SERIAL)
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
bool SerialServiceProvider::initService()
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

  return true;
}

/**
 * handle processing of received serial data based on its type
 *
 * @param serial_event_t* se
 */
void SerialServiceProvider::processSerial(serial_event_t *se)
{
  if( nullptr != se ){

    if( SERIAL_TYPE_UART == se->type && nullptr != se->serial ){

      int32_t available = se->serial->available();
      pdiutil::string recvdata = "";

      while (se->serial->available())
      {
        char c = se->serial->read();
        // break command on line ending
        if (c == '\n') {
          
          break;
        }else{
          
          recvdata += c;
        }
        __i_dvc_ctrl.wait(1);
      }

      LogFmtI("serial uart recv (%d) : %s\n", recvdata.size(), recvdata.c_str());

      // process and execute if command has provided
      #ifdef ENABLE_CMD_SERVICE
      cmd_result_t result = __cmd_service.executeCommand(&recvdata);
      #endif

      // make sure to flush serial if no more data available
      __i_dvc_ctrl.wait(1);
      if(!se->serial->available()) se->serial->flush();
    }
  }
}

SerialServiceProvider __serial_service;

#endif