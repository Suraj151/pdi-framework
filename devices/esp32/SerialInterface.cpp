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
#if ARDUINO_USB_CDC_ON_BOOT
, m_cdcserial(nullptr)
#endif
{
}

#if ARDUINO_USB_CDC_ON_BOOT
UARTSerial::UARTSerial(HWCDC& cdcserial) : m_connected(false), m_port(0), m_speed(115200), m_hwserial(Serial0), m_cdcserial(&cdcserial)
{
}
#endif

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

#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) m_cdcserial->begin(speed);
  else
#endif
  m_hwserial.begin(speed);
  m_connected = true;
  return 1;
}

/**
 * disconnect
 */
int16_t UARTSerial::disconnect()
{
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) m_cdcserial->end();
  else
#endif
  m_hwserial.end();
  m_connected = false;
  return 0;
}

/**
 * write
 */
int32_t UARTSerial::write(uint8_t c)
{
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) return m_cdcserial->write(c);
#endif
  return m_hwserial.write(c);
}

/**
 * write
 */
int32_t UARTSerial::write(const uint8_t *c_str)
{
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) return m_cdcserial->write((const char*)c_str);
#endif
  return m_hwserial.write((const char*)c_str);
}

/**
 * write
 */
int32_t UARTSerial::write(const uint8_t *c_str, uint32_t size)
{
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) return m_cdcserial->write(c_str, size);
#endif
  return m_hwserial.write(c_str, size);
}

/**
 * write
 */
int32_t UARTSerial::write_ro(const char *c_str)
{
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) return m_cdcserial->print(c_str);
#endif
  return m_hwserial.print(c_str);
}

/**
 * read
 */
uint8_t UARTSerial::read()
{
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) return m_cdcserial->read();
#endif
  return m_hwserial.read();
}

/**
 * read
 */
int32_t UARTSerial::read(uint8_t *buf, uint32_t size)
{
  int32_t count = 0;
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) {
    for (; count < size && m_cdcserial->available(); ++count)
    {
      buf[count] = m_cdcserial->read();
    }
    return count;
  }
#endif
  for (; count < size && m_hwserial.available(); ++count)
  {
    buf[count] = m_hwserial.read();
  }
  return count;
}

/**
 * available
 */
int32_t UARTSerial::available()
{
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) return m_cdcserial->available();
#endif
  return m_hwserial.available();
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
#if ARDUINO_USB_CDC_ON_BOOT
  if (m_cdcserial) {
    m_cdcserial->flush();
    while (m_cdcserial->available() > 0) m_cdcserial->read();
    return;
  }
#endif
  m_hwserial.flush();
  while (m_hwserial.available() > 0) m_hwserial.read();
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
  __sprintf(tembuff, "%lu", millis());
  
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
  Now polled via DeviceControlInterface::handleEvents() to also cover HWCDC
  (USB CDC on boot) where Arduino core does not auto-invoke serialEvent.
*/
// void serialEvent() {
//
//   // check for uart serial data available if any
//   if (__serial_uart.available()) {
//     serial_event_t e(SERIAL_IFACE_UART, SERIAL_IFACE_CMD, &__serial_uart);
//     __utl_event.execute_event(EVENT_SERIAL_AVAILABLE, &e);
//   }
// }