/********************************* Driver Gpio *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The pins a driver's bus holds, answered for every port in one place so each
port's exceptional gpio check only has to ask.

Author          : Suraj I.
created Date    : 20th Oct 2026
******************************************************************************/
#include <config/Config.h>

#ifdef ENABLE_DRIVER_CORE

#include <interface/pdi.h>
#include <service_provider/transport/SerialServiceProvider.h>

/**
 * Whether a line in /etc/devices.conf holds the hardware pin for a driver's
 * bus, so gpio operations leave it alone.
 */
bool iDeviceControlInterface::isDriverGpio(gpio_id_t hwpin)
{
  DeviceManager *mgr = __serial_service.devices();
  return nullptr != mgr && mgr->holdsPin((uint8_t)hwpin);
}

#endif
