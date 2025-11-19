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
      // if( SERIAL_TYPE_UART == serialevent->source ){

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

    if( SERIAL_IFACE_UART == se->source && SERIAL_IFACE_CMD == se->dest ){

      // process and execute if command has provided
      #ifdef ENABLE_CMD_SERVICE
      iTerminalInterface *terminal = __cmd_service.getTerminal();
      if( nullptr == terminal ){
        __cmd_service.useTerminal(se->serial);
        terminal = __cmd_service.getTerminal();
      }

      // Currently supporting multiple client (tcp, telnet, serial, ssh) for command service.
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

/**
 * append available serial json data
 *
 */
void SerialServiceProvider::appendSerialJsonPayload(pdiutil::string &_payload, pdiutil::vector<pdiutil::string> *allowedlist){

  _payload += "{\"";
  _payload += SERIAL_PAYLOAD_DATA_KEY;
  _payload += "\":{";

  bool _remove_comma = false;
  for (uint8_t _port = 0; _port < MAX_SERIAL_PORT; _port++) {

    if( this->isAllowedSerialPort(_port, allowedlist) ){

      _payload += "\"UART";
      _payload += pdiutil::to_string(_port);
      _payload += "\":{\"";
      _payload += SERIAL_PAYLOAD_MODE_KEY;
      _payload += "\":";
      _payload += pdiutil::to_string((int)SERIAL_READ);
      _payload += ",\"";
      _payload += SERIAL_PAYLOAD_VALUE_KEY;
      _payload += "\":";
      _payload += "\"\"";  // todo read data from serial
      _payload += "},";

      _remove_comma = true;
    }
  }

  if( _remove_comma ){
    _payload.pop_back(); // remove last comma
  }

  _payload += "}}";
}

/**
 * handle received serial json data
 *
 */
void SerialServiceProvider::applySerialJsonPayload(char *_payload, uint16_t _payload_length, pdiutil::vector<pdiutil::string> *allowedlist){

  LogFmtI("Applying Serial from Json Payload : %s\n", _payload);

  if(
    0 <= __strstr( _payload, (char*)SERIAL_PAYLOAD_DATA_KEY, _payload_length - strlen(SERIAL_PAYLOAD_DATA_KEY) ) &&
    0 <= __strstr( _payload, (char*)SERIAL_PAYLOAD_MODE_KEY, _payload_length - strlen(SERIAL_PAYLOAD_MODE_KEY) ) &&
    0 <= __strstr( _payload, (char*)SERIAL_PAYLOAD_VALUE_KEY, _payload_length - strlen(SERIAL_PAYLOAD_VALUE_KEY) )
  ){

    int _port_data_max_len = MAX_SERIAL_DATA_LENGTH_FOR_IOT, _port_keys_max_len = 10;

    char _port_label_uppercase[_port_keys_max_len];
    char _port_label_lowercase[_port_keys_max_len];
    char _port_json_data[_port_data_max_len]; 
    char _port_mode[_port_keys_max_len]; 
    char _port_data[_port_data_max_len];

    // Decode the pin mode and value
    for (uint8_t _port = 0; _port < MAX_SERIAL_PORT; _port++) {

      uint8_t _port_label_n = _port;

      memset( _port_label_uppercase, 0, _port_keys_max_len);
      memset( _port_label_lowercase, 0, _port_keys_max_len);
      memset( _port_json_data, 0, _port_data_max_len);
      memset( _port_mode, 0, _port_keys_max_len); 
      memset( _port_data, 0, _port_data_max_len);

      // currently support for uart only
      __appendUintToBuff(_port_label_uppercase, "UART%d", _port_label_n, _port_keys_max_len-1);
      __appendUintToBuff(_port_label_lowercase, "uart%d", _port_label_n, _port_keys_max_len-1);

      if( __get_from_json( _payload, _port_label_uppercase, _port_json_data, _port_data_max_len ) || __get_from_json( _payload, _port_label_lowercase, _port_json_data, _port_data_max_len ) ){

        if( allowedlist != nullptr ){

          bool _is_allowed = false;

          for( size_t i=0; i < allowedlist->size(); i++ ){

            if( __are_str_equals( allowedlist->at(i).c_str(), _port_label_uppercase, strlen( _port_label_uppercase ) ) ||
                __are_str_equals( allowedlist->at(i).c_str(), _port_label_lowercase, strlen( _port_label_lowercase ) ) ){

              _is_allowed = true;
              break;
            }
          }

          if( !_is_allowed ){

            continue;
          }
        }

        if( __get_from_json( _port_json_data, (char*)SERIAL_PAYLOAD_MODE_KEY, _port_mode, _port_keys_max_len ) ){

          if( __get_from_json( _port_json_data, (char*)SERIAL_PAYLOAD_VALUE_KEY, _port_data, _port_data_max_len ) ){

            LogFmtI("Applying to : %s, mode : %s, value : %s\n", _port_label_uppercase, _port_mode, _port_data);

            uint8_t _mode = StringToUint8( _port_mode, _port_keys_max_len );
            // todo send data on serial
          }
        }
      }
    }
  }
}

/**
 * check if serial port is allowed in allowedlist
 *
 * @param uint8_t _port
 * @param pdiutil::vector<pdiutil::string>* allowedlist
 * @return bool
 */
bool SerialServiceProvider::isAllowedSerialPort(uint8_t _port, pdiutil::vector<pdiutil::string> *allowedlist){

  int _port_keys_max_len = 10;

  char _port_label_uppercase[_port_keys_max_len];
  char _port_label_lowercase[_port_keys_max_len];

  memset( _port_label_uppercase, 0, _port_keys_max_len);
  memset( _port_label_lowercase, 0, _port_keys_max_len);

  // currently support for uart only
  __appendUintToBuff(_port_label_uppercase, "UART%d", _port, _port_keys_max_len-1);
  __appendUintToBuff(_port_label_lowercase, "uart%d", _port, _port_keys_max_len-1);

  if( allowedlist != nullptr ){

    for( size_t i=0; i < allowedlist->size(); i++ ){

      if( __are_str_equals( allowedlist->at(i).c_str(), _port_label_uppercase, strlen( _port_label_uppercase ) ) ||
          __are_str_equals( allowedlist->at(i).c_str(), _port_label_lowercase, strlen( _port_label_lowercase ) ) ){

        return true;
      }
    }

    return false;
  }

  return true;
}

SerialServiceProvider __serial_service;

#endif