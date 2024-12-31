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


/**
* available serial types
*/
typedef enum serial_types{

  SERIAL_TYPE_UART = 0,
  SERIAL_TYPE_I2C,
  SERIAL_TYPE_SPI,
  SERIAL_TYPE_CAN,
  SERIAL_TYPE_MAX
} serial_types_t;


// forward declaration
class iSerialInterface;

/**
* serial event wrapper struct
*/
typedef struct SerialEvent{
  serial_types_t type;
  iSerialInterface *serial;
  SerialEvent() : type(SERIAL_TYPE_MAX), serial(nullptr) {}
  SerialEvent(serial_types_t _type, iSerialInterface *_serial) : type(_type), serial(_serial) {}
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
  static iSerialInterface *instances[SERIAL_TYPE_MAX];

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
  static iSerialInterface *getSerialInstance(serial_types_t type) {

    if( SERIAL_TYPE_MAX > type ){
      return instances[type];
    } 
    return nullptr; 
  }
};

#endif
