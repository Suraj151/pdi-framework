/****************************** Utility Interface *****************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_DEVICE_UTILITY_INTERFACE_H_
#define _I_DEVICE_UTILITY_INTERFACE_H_

#include "DataTypeDef.h"

/**
 * iUtilityInterface class
 * 
 * This interface should be defined by target device to satisfy the respected api
 */
class iUtilityInterface
{

public:
  /**
   * iUtilityInterface destructor.
   */
  virtual ~iUtilityInterface(){}

  virtual void wait(uint64_t timeoutms) = 0;
  virtual uint32_t millis_now() = 0;
};

#endif
