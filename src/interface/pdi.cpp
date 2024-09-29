/*********************** Device Control Interface *****************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#include "pdi.h"

#ifdef MOCK_DEVICE_TEST
#include "../../devices/mockdevice/mockdevice_pdi.cpp"
#else
#include "../../devices/esp8266/esp8266_pdi.cpp"
#endif
