/****************************** Serial Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_SERIAL_INTERFACE_H_
#define _I_SERIAL_INTERFACE_H_

#include <interface/interface_includes.h>
#include <interface/pdi/middlewares/iClientInterface.h>

// forward declaration
class iSerialInterface;

/**
* serial event wrapper struct
*/
typedef struct SerialEvent{
  serial_iface_t source;
  serial_iface_t dest;
  iSerialInterface *serial;
  void *data;
  uint16_t size;
  SerialEvent() : 
    source(SERIAL_IFACE_MAX), 
    dest(SERIAL_IFACE_MAX), 
    serial(nullptr),
    data(nullptr),
    size(0) {}
  SerialEvent(serial_iface_t _src, serial_iface_t _dest, iSerialInterface *_serial, void* _data = nullptr, uint16_t _size = 0) : 
    source(_src), 
    dest(_dest), 
    serial(_serial),
    data(_data),
    size(_size) {}
} serial_event_t;


/**
 * iSerialInterface class
 */
class iSerialInterface : public iClientInterface
{

public:

  /**
   * serial interface instances.
   */
  static iSerialInterface *instances[SERIAL_IFACE_MAX];

  /**
   * iSerialInterface constructor.
   */
  iSerialInterface() {

  }

  /**
   * iSerialInterface destructor.
   */
  virtual ~iSerialInterface() {}

  /**
   * Get instance of serial.
   * Before using this api, user must make sure of creating respective type instance
   */
  static iSerialInterface *getSerialInstance(serial_iface_t type) {

    if( SERIAL_IFACE_MAX > type ){
      return instances[type];
    } 
    return nullptr; 
  }
};

/**
 * iSerialTerminalInterface class
 */
using iSerialTerminalInterface = iSerialInterface;

#endif
