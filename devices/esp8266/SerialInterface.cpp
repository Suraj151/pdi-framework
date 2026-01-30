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
iSerialInterface* iSerialInterface::instances[SERIAL_IFACE_MAX] = {
  &__serial_uart // SERIAL_IFACE_UART
  ,&__serial_uart1 // SERIAL_IFACE_UART1
  ,nullptr // SERIAL_IFACE_I2C
  ,nullptr // SERIAL_IFACE_I2C1
  ,nullptr // SERIAL_IFACE_SPI
  ,nullptr // SERIAL_IFACE_SPI1
  ,nullptr // SERIAL_IFACE_CAN
  ,nullptr // SERIAL_IFACE_CAN1
  ,nullptr // SERIAL_IFACE_CMD
  ,nullptr // SERIAL_IFACE_IOT
};

/**
 * UARTSerial constructor.
 */
UARTSerial::UARTSerial(HardwareSerial& hwserial) : m_connected(false), m_port(0), m_speed(115200), m_hwserial(hwserial)
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
  m_port = port;
  m_speed = speed;

  m_hwserial.begin(speed);
  m_connected = true;
  return 1;
}

/**
 * disconnect
 */
int16_t UARTSerial::disconnect()
{
  m_hwserial.end();
  m_connected = false;
  return 0;
}

/**
 * write
 */
int32_t UARTSerial::write(uint8_t c)
{
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_lock();
  #endif

  int32_t ret = m_hwserial.write(c);

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_unlock();
  #endif

  return ret;
}

/**
 * write
 */
int32_t UARTSerial::write(const uint8_t *c_str)
{
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_lock();
  #endif

  int32_t ret = m_hwserial.write((const char*)c_str);

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_unlock();
  #endif

  return ret;
}

/**
 * write
 */
int32_t UARTSerial::write(const uint8_t *c_str, uint32_t size)
{
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_lock();
  #endif

  int32_t ret = m_hwserial.write(c_str, size);

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_unlock();
  #endif

  return ret;
}

/**
 * write
 */
int32_t UARTSerial::write_ro(const char *c_str)
{
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_lock();
  #endif

  int32_t ret = m_hwserial.print(c_str);
  
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_unlock();
  #endif

  return ret;
}

/**
 * read
 */
uint8_t UARTSerial::read()
{
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_lock();
  #endif

  uint8_t ret = m_hwserial.read();

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_unlock();
  #endif
  
  return ret;
}

/**
 * read
 */
int32_t UARTSerial::read(uint8_t *buf, uint32_t size)
{
  uint32_t count = 0;

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_lock();
  #endif

  for (; count < size && m_hwserial.available(); ++count)
  {
    buf[count] = m_hwserial.read();
  }

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_unlock();
  #endif

  return count;
}

/**
 * available
 */
int32_t UARTSerial::available()
{
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_lock();
  #endif

  int32_t ret = m_hwserial.available();

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_unlock();
  #endif

  return ret;
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
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_lock();
  #endif

  m_hwserial.flush();
  while (m_hwserial.available() > 0) m_hwserial.read(); 

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  m_mutex.critical_unlock();
  #endif  
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
  snprintf(tembuff, sizeof(tembuff), "%lu", millis());
  
  write('[');
  write_pad(tembuff, 10, true);
  write(']');
  
  return this;
}

UARTSerial __serial_uart(Serial);
UARTSerial __serial_uart1(Serial1);

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {

  // check for uart serial data available if any
  if (__serial_uart.available()) {
    serial_event_t e(SERIAL_IFACE_UART, SERIAL_IFACE_CMD, &__serial_uart);
    __utl_event.execute_event(EVENT_SERIAL_AVAILABLE, &e);
  }
}