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
  SERIAL_IFACE_MAX
} serial_iface_t;

/**
* available serial modes
*/
enum SERIAL_MODE {
  SERIAL_MODE_OFF = GPIO_MODE_MAX,
  SERIAL_WRITE,
  SERIAL_READ,
  SERIAL_MODE_MAX
};


#endif
