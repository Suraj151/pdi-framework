/****************************** Serial Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _SERIAL_INTERFACE_H_
#define _SERIAL_INTERFACE_H_

#include "esp8266.h"
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
  UARTSerial(HardwareSerial& hwserial);
  /**
   * UARTSerial destructor.
   */
  ~UARTSerial();

  // connect/disconnect api
  int16_t connect(uint16_t port, uint64_t speed) override;
  int16_t disconnect() override;

  // data sending api
  int32_t write(uint8_t c) override;
  int32_t write(const uint8_t *c_str) override;
  int32_t write(const uint8_t *c_str, uint32_t size) override;
  int32_t write_ro(const char *c_str) override;

  // received data read api
  uint8_t read() override;
  int32_t read(uint8_t *buf, uint32_t size) override;

  // useful api
  int32_t available() override;
  int8_t connected() override;
  void setTimeout(uint32_t timeout) override;
  void flush() override;

  // terminal api
  iTerminalInterface* with_timestamp() override;

private:
  bool m_connected;
  uint16_t m_port;
  uint64_t m_speed;
  HardwareSerial& m_hwserial;
};

extern UARTSerial __serial_uart;
extern UARTSerial __serial_uart1;


#endif
