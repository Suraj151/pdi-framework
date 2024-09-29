/*********************** Device Control Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _PORTABLE_DEVICE_INTERFACE_H_
#define _PORTABLE_DEVICE_INTERFACE_H_

#include <config/Config.h>

#ifdef MOCK_DEVICE_TEST
#include "../../devices/mockdevice/mockdevice_pdi.h"
#else
#include "../../devices/esp8266/esp8266_pdi.h"
#endif

#include <interface/pdi/iDeviceIotInterface.h>

#endif  // _PORTABLE_DEVICE_INTERFACE_H_
