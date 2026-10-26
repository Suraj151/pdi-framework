/****************************** Built-in Drivers ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The drivers the framework carries, registered once with the serial service's
device manager. Each one is compiled in only where the board has the bus it
sits on.

Author          : Suraj I.
created Date    : 19th Oct 2026
******************************************************************************/

#ifndef _BUILT_IN_DRIVERS_H_
#define _BUILT_IN_DRIVERS_H_

#include <config/Config.h>

#ifdef ENABLE_DRIVER_CORE

/**
 * Registers every driver this build carries; registering twice is harmless.
 */
void registerBuiltInDrivers();

#endif

#endif
