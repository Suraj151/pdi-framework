/****************************** Built-in Drivers ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 19th Oct 2026
******************************************************************************/

#include "BuiltInDrivers.h"

#ifdef ENABLE_DRIVER_CORE

#include <service_provider/transport/DeviceManager.h>

#ifdef ENABLE_I2S_BUS
#include "Inmp441Driver.h"
#endif

#ifdef ENABLE_I2C_BUS
#include "At24Driver.h"
#endif

#ifdef ENABLE_SPI_BUS
#include "MmcSpiDriver.h"
#endif

/**
 * Registers every driver this build carries; registering twice is harmless.
 */
void registerBuiltInDrivers()
{
#ifdef ENABLE_I2S_BUS
  Inmp441Driver::registerDriver();
#endif
#ifdef ENABLE_I2C_BUS
  At24Driver::registerDriver();
#endif
#ifdef ENABLE_SPI_BUS
  MmcSpiDriver::registerDriver();
#endif
}

#endif
