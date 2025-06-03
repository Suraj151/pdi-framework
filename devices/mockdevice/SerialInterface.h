/****************************** Serial Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _MOCKDEVICE_SERIAL_INTERFACE_H_
#define _MOCKDEVICE_SERIAL_INTERFACE_H_

#include "mockdevice.h"
#include <interface/pdi/modules/serial/iSerialInterface.h>

/**
 * UARTSerial class
 */
class UARTSerial : public iSerialInterface
{

public:
  /**
   * UARTSerial constructor.
   */
  UARTSerial();
  /**
   * UARTSerial destructor.
   */
  ~UARTSerial();

  // connect/disconnect api
  int16_t connect(uint16_t port, uint64_t speed) override{ return 0; }
  int16_t disconnect() override { return 0; }

  // data sending api
  uint32_t write(uint8_t c) override { return 0; }
  uint32_t write(const uint8_t *c_str) override { return 0; }
  uint32_t write(const uint8_t *c_str, uint32_t size) override { return 0; }

  // received data read api
  uint8_t read() override { return 0; }
  uint32_t read(uint8_t *buf, uint32_t size) override { return 0; }

  // useful api
  int32_t available() override { return -1; }
  int8_t connected() override { return 0; }
  void setTimeout(uint32_t timeout) override {}
  void flush() override {}
};


#endif
