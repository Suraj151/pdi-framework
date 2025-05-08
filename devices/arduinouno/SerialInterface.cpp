/****************************** Serial Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "SerialInterface.h"


// extern int __heap_start, *__brkval; int v; 
// int ramavail = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
// LogFmtI("available ram %d, %d\n", ramavail, sizeof(task_t));


// initialize instances with respective type or nullptr
iSerialInterface* iSerialInterface::instances[SERIAL_TYPE_MAX] = {
  &__serial_uart // SERIAL_TYPE_UART
  ,nullptr // SERIAL_TYPE_I2C
  ,nullptr // SERIAL_TYPE_SPI
  ,nullptr // SERIAL_TYPE_CAN
};

/**
 * UARTSerial constructor.
 */
UARTSerial::UARTSerial() : m_connected(false)
{
}

/**
 * UARTSerial destructor.
 */
UARTSerial::~UARTSerial()
{
}

/**
 * connect
 */
int16_t UARTSerial::connect(uint16_t port, uint64_t speed)
{
  Serial.begin(speed);
  m_connected = true;
  return 1;
}

/**
 * disconnect
 */
int16_t UARTSerial::disconnect()
{
  Serial.end();
  m_connected = false;
  return 0;
}

/**
 * write
 */
uint32_t UARTSerial::write(uint8_t c)
{
  return Serial.write(c);
}

/**
 * write
 */
uint32_t UARTSerial::write(const uint8_t *c_str)
{
  return Serial.write((const char*)c_str);
}

/**
 * write
 */
uint32_t UARTSerial::write(const uint8_t *c_str, uint32_t size)
{
  return Serial.write(c_str, size);
}

/**
 * write
 */
uint32_t UARTSerial::write_ro(const char *c_str)
{
  return Serial.print((__FlashStringHelper*)c_str);
}

/**
 * read
 */
uint8_t UARTSerial::read()
{
  return Serial.read();
}

/**
 * read
 */
uint32_t UARTSerial::read(uint8_t *buf, uint32_t size)
{
  uint32_t count = 0;
  for (; count < size && Serial.available(); ++count)
  {
    buf[count] = Serial.read();
  }
  return count;
}

/**
 * available
 */
int32_t UARTSerial::available()
{
  return Serial.available();
}

/**
 * connected
 */
int8_t UARTSerial::connected()
{
  return m_connected;
}

/**
 * setTimeout
 */
void UARTSerial::setTimeout(uint32_t timeout)
{
}

/**
 * flush
 */
void UARTSerial::flush()
{
  Serial.flush();
  while (Serial.available() > 0) Serial.read(); 
}

/**
 * return the new client interface object
 */
iClientInterface *UARTSerial::getNewInstance()
{
  return iSerialInterface::instances[SERIAL_TYPE_UART];
}

/**
 * @brief With timestamp will print timestamp first
 * derived class should implement this function to print timestamp
 * @param None
 * @return this
 */
iTerminalInterface* UARTSerial::with_timestamp()
{
  char tembuff[15];
  memset(tembuff, 0, 15);
  sprintf(tembuff, "%ld", millis());
  
  write('[');
  write_pad(tembuff, 10, true);
  write(']');
  
  return this;
}

UARTSerial __serial_uart;

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {

  // check for uart serial data available if any
  if (Serial.available()) {
    serial_event_t e(SERIAL_TYPE_UART, &__serial_uart);
    __utl_event.execute_event(EVENT_SERIAL_AVAILABLE, &e);
    __serial_uart.flush();
  }
}