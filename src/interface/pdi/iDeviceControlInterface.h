/*********************** Device Control Interface *****************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_DEVICE_CONTROL_INTERFACE_H_
#define _I_DEVICE_CONTROL_INTERFACE_H_

#include <interface/interface_includes.h>
#include <config/GpioConfig.h>

// forward declaration of derived class for this interface
class DeviceControlInterface;

/**
 * iDeviceControlInterface class
 * 
 * This class handles more about hardware things of device
 * So this can be utilized in utility as well
 */
class iDeviceControlInterface : public iUtilityInterface
{

public:
  /**
   * iDeviceControlInterface constructor.
   */
  iDeviceControlInterface(){}

  /**
   * iDeviceControlInterface destructor.
   */
  virtual ~iDeviceControlInterface(){}

  // GPIO methods
  virtual void gpioWrite( GPIO_MODE mode, gpio_id_t pin, gpio_val_t value ) = 0;
  virtual gpio_val_t gpioRead( GPIO_MODE mode, gpio_id_t pin ) = 0;

  // device control methods
  virtual void resetDevice() = 0;
  virtual void restartDevice() = 0;

  // misc methods
  virtual void eraseConfig() = 0;
  virtual uint32_t getDeviceId() = 0;
  virtual void deviceWdtFeed() = 0;
  virtual bool isDeviceFactoryRequested() = 0;

  // below are the utility interface dependencies which needs to be satisfy
  // virtual void wait(uint64_t timeoutms) = 0;
  // virtual uint32_t millis_now() = 0;
};

// derived class must define this
extern DeviceControlInterface __i_dvc_ctrl;

#endif
