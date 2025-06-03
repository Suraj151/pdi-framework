/******************************* GPIO Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_GPIO_INTERFACE_H_
#define _I_GPIO_INTERFACE_H_

#include <config/GpioConfig.h>

// currently 100 ms is kept as lowest blink time
#define GPIO_DIGITAL_BLINK_MIN_DURATION_MS 100

/**
 * iGpioBlinkerInterface class
 * 
 * This interface handles more about GPIO blink with given gpio pin and duration
 */
class iGpioBlinkerInterface
{

public:
  /**
   * iGpioBlinkerInterface constructor.
   */
  iGpioBlinkerInterface(){}

  /**
   * iGpioBlinkerInterface destructor.
   */
  virtual ~iGpioBlinkerInterface(){}

  // GPIO Blinker config methods
  virtual void setConfig( gpio_id_t pin, gpio_val_t duration ) = 0;
  virtual void updateConfig( gpio_id_t pin, gpio_val_t duration ) = 0;

  // GPIO Blinker control methods
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual bool isRunning() = 0;
};


/**
 * iGpioInterface class
 * 
 * This interface handles more about GPIO operations of device
 */
class iGpioInterface
{

public:
  /**
   * iGpioInterface constructor.
   */
  iGpioInterface(){}

  /**
   * iGpioInterface destructor.
   */
  virtual ~iGpioInterface(){}

  // GPIO methods
  virtual void gpioMode( GPIO_MODE mode, gpio_id_t pin ) = 0;
  virtual void gpioWrite( GPIO_MODE mode, gpio_id_t pin, gpio_val_t value ) = 0;
  virtual gpio_val_t gpioRead( GPIO_MODE mode, gpio_id_t pin ) = 0;
  virtual gpio_id_t gpioFromPinMap( gpio_id_t pin, bool isAnalog=false ) = 0;
  virtual bool isExceptionalGpio( gpio_id_t pin ) = 0;

  // GPIO Blinker object create/destroy methods
  virtual iGpioBlinkerInterface *createGpioBlinkerInstance(gpio_id_t pin, gpio_val_t duration) = 0;
  virtual void releaseGpioBlinkerInstance(iGpioBlinkerInterface *instance) = 0;
};

#endif
