/*********************** Device Control Interface *****************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _DEVICE_CONTROL_INTERFACE_H_
#define _DEVICE_CONTROL_INTERFACE_H_

#include "esp8266.h"
#include <interface/pdi/iDeviceControlInterface.h>
#include "LoggerInterface.h"

/**
 * DeviceControlInterface class
 */
class DeviceControlInterface: public iDeviceControlInterface
{

public:
  /**
   * DeviceControlInterface constructor.
   */
  DeviceControlInterface();

  /**
   * DeviceControlInterface destructor.
   */
  ~DeviceControlInterface();

  // GPIO methods
  void gpioWrite( GPIO_MODE mode, gpio_id_t pin, gpio_val_t value ) override;
  gpio_val_t gpioRead( GPIO_MODE mode, gpio_id_t pin ) override;

  // device control methods
  void resetDevice() override;
  void restartDevice() override;

  // misc methods
  void eraseConfig() override;
  uint32_t getDeviceId() override;
  void deviceWdtFeed() override;
  bool isDeviceFactoryRequested() override;

  // util methods
  void wait(uint64_t timeoutms) override;
  uint32_t millis_now() override;
  void log(logger_type_t log_type, const char *content) override;
  void yield() override;
};

#endif
