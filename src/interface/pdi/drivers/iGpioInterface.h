/******************************* GPIO Interface ******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_GPIO_INTERFACE_H_
#define _I_GPIO_INTERFACE_H_

#include <config/GpioConfig.h>

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
};

#endif
