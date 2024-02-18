/************************ watchdog timer Interface ***************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_WATCHDOG_TIMER_INTERFACE_H_
#define _I_WATCHDOG_TIMER_INTERFACE_H_

#include <utility/DataTypeDef.h>

/**
 * iWdtInterface class
 * 
 * This interface handles watchdog timer operations of device
 */
class iWdtInterface
{

public:
  /**
   * iWdtInterface constructor.
   */
  iWdtInterface(){}

  /**
   * iWdtInterface destructor.
   */
  virtual ~iWdtInterface(){}

  // Wdt methods
  virtual void enableWdt(uint8_t mode_if_any=0) = 0;
  virtual void disableWdt() = 0;
  virtual void feedWdt() = 0;
};

#endif
