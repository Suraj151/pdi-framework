/*********************** Device Control Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_DEVICE_CONTROL_INTERFACE_H_
#define _I_DEVICE_CONTROL_INTERFACE_H_

#include <interface/interface_includes.h>
#include <interface/pdi/drivers/iGpioInterface.h>
#include <interface/pdi/drivers/iWdtInterface.h>
#include "iUpgradeInterface.h"

// forward declaration of derived class for this interface
class DeviceControlInterface;

/**
 * iDeviceControlInterface class
 *
 * This interface handles more about hardware things of device
 * So this can be utilized in utility as well
 */
class iDeviceControlInterface : public iGpioInterface,
                                public iWdtInterface,
                                public iUtilityInterface,
                                public iUpgradeInterface
{

public:
  /**
   * iDeviceControlInterface constructor.
   */
  iDeviceControlInterface() {}

  /**
   * iDeviceControlInterface destructor.
   */
  virtual ~iDeviceControlInterface() {}

  // below are the gpio interface dependencies which needs to be satisfy
  // GPIO methods
  // virtual void gpioMode( GPIO_MODE mode, gpio_id_t pin ) = 0;
  // virtual void gpioWrite( GPIO_MODE mode, gpio_id_t pin, gpio_val_t value ) = 0;
  // virtual gpio_val_t gpioRead( GPIO_MODE mode, gpio_id_t pin ) = 0;
  // virtual gpio_id_t gpioFromPinMap( gpio_id_t pin ) = 0;
  // virtual bool isExceptionalGpio( gpio_id_t pin ) = 0;
  // virtual iGpioBlinkerInterface *createGpioBlinkerInstance(gpio_id_t pin, gpio_val_t duration) = 0;
  // virtual void releaseGpioBlinkerInstance(iGpioBlinkerInterface *instance) = 0;

  // device control methods
  virtual void initDeviceSpecificFeatures() = 0;
  virtual void resetDevice() = 0;
  virtual void restartDevice() = 0;

  // below are the wdt interface dependencies which needs to be satisfy
  // virtual void enableWdt(uint8_t mode_if_any) = 0;
  // virtual void disableWdt() = 0;
  // virtual void feedWdt() = 0;

  // misc methods
  virtual void eraseConfig() = 0;
  virtual uint32_t getDeviceId() = 0;
  virtual std::string getDeviceMac() = 0;
  virtual bool isDeviceFactoryRequested() = 0;

  // below are the utility interface dependencies which needs to be satisfy
  // virtual void wait(uint64_t timeoutms) = 0;
  // virtual uint32_t millis_now() = 0;
  // virtual void log(const char *format, logger_type_t log_type, ...) = 0;
  // virtual void yield() = 0;

  // upgrade api
  // virtual upgrade_status_t Upgrade(const char *path, const char *version) = 0;
};

// derived class must define this
extern DeviceControlInterface __i_dvc_ctrl;

#endif
