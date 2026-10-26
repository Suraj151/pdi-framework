/************************** Serial Config page *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2025
******************************************************************************/
#ifndef _SERIAL_CONFIG_H_
#define _SERIAL_CONFIG_H_

#include "Common.h"
#include "GpioConfig.h"

#define MAX_SERIAL_PORT 2
#define MAX_SERIAL_DATA_LENGTH_FOR_IOT_PAYLOAD 150

#define SERIAL_INTERFACE_UART  "uart"
#define SERIAL_INTERFACE_CAN   "can"
#define SERIAL_INTERFACE_I2C   "i2c"
#define SERIAL_INTERFACE_SPI   "spi"
#define SERIAL_INTERFACE_I2S   "i2s"
#define SERIAL_INTERFACE_USB   "usb"

#define SERIAL_PAYLOAD_DATA_KEY    GPIO_PAYLOAD_DATA_KEY
#define SERIAL_PAYLOAD_MODE_KEY    GPIO_PAYLOAD_MODE_KEY
#define SERIAL_PAYLOAD_VALUE_KEY   GPIO_PAYLOAD_VALUE_KEY

/**
* available serial interfaces
*/
typedef enum serial_iface{

  SERIAL_IFACE_UART = 0,
  SERIAL_IFACE_UART1,
  SERIAL_IFACE_I2C,
  SERIAL_IFACE_I2C1,
  SERIAL_IFACE_SPI,
  SERIAL_IFACE_SPI1,
  SERIAL_IFACE_CAN,
  SERIAL_IFACE_CAN1,
  SERIAL_IFACE_CMD,
  SERIAL_IFACE_IOT,
  SERIAL_IFACE_I2S,
  SERIAL_IFACE_USB_HOST,
  SERIAL_IFACE_USB_DEVICE,
  SERIAL_IFACE_MAX
} serial_iface_t;

/**
 * The first interface of a kind, which every other one of that kind shares
 * its name and its drivers with.
 */
inline serial_iface_t serialFamily(serial_iface_t iface) {
  switch( iface ){
    case SERIAL_IFACE_UART1: return SERIAL_IFACE_UART;
    case SERIAL_IFACE_I2C1:  return SERIAL_IFACE_I2C;
    case SERIAL_IFACE_SPI1:  return SERIAL_IFACE_SPI;
    case SERIAL_IFACE_CAN1:  return SERIAL_IFACE_CAN;
    default:                 return iface;
  }
}

/**
 * The name of an interface's kind, copied out of read only memory; false for
 * one that is not a bus.
 */
inline bool serialInterfaceName(serial_iface_t iface, pdiutil::string &out) {
  switch( serialFamily(iface) ){
    case SERIAL_IFACE_UART:     out = CHARPTR_WRAP(SERIAL_INTERFACE_UART); return true;
    case SERIAL_IFACE_I2C:      out = CHARPTR_WRAP(SERIAL_INTERFACE_I2C);  return true;
    case SERIAL_IFACE_SPI:      out = CHARPTR_WRAP(SERIAL_INTERFACE_SPI);  return true;
    case SERIAL_IFACE_CAN:      out = CHARPTR_WRAP(SERIAL_INTERFACE_CAN);  return true;
    case SERIAL_IFACE_I2S:      out = CHARPTR_WRAP(SERIAL_INTERFACE_I2S);  return true;
    case SERIAL_IFACE_USB_HOST: out = CHARPTR_WRAP(SERIAL_INTERFACE_USB);  return true;
    default:                    out.clear(); return false;
  }
}

/**
 * Which one of its kind an interface is, 0 for the first.
 */
inline uint8_t serialInterfaceIndex(serial_iface_t iface) {
  return (uint8_t)(iface - serialFamily(iface));
}

/**
* available serial modes
*/
enum SERIAL_MODE : uint8_t {
  SERIAL_MODE_OFF = GPIO_MODE_MAX,
  SERIAL_WRITE,
  SERIAL_READ,
  SERIAL_MODE_MAX
};

/**
* how an i2s interface is wired and clocked; a pin of -1 is not used
*/
struct serial_i2s_config_t {
  serial_i2s_config_t() : m_bclk(-1), m_ws(-1), m_din(-1), m_dout(-1), m_rate(16000), m_bits(16), m_capture(true) {}

  int8_t m_bclk;
  int8_t m_ws;
  int8_t m_din;
  int8_t m_dout;
  uint32_t m_rate;
  uint8_t m_bits;
  bool m_capture;
};

/**
* a device attached to a usb host port, as its descriptors name it
*/
struct usb_device_info_t {
  usb_device_info_t() : m_address(0), m_vid(0), m_pid(0), m_class(0), m_subclass(0), m_protocol(0) {}

  uint8_t m_address;
  uint16_t m_vid;
  uint16_t m_pid;
  uint8_t m_class;
  uint8_t m_subclass;
  uint8_t m_protocol;
};


#endif
