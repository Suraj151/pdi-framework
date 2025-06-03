/*********************** Device Control Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#include "pdi.h"

#if defined(DEVICE_ESP32)
#include "../../devices/esp32/esp32_pdi.cpp"
#elif defined(DEVICE_ESP8266)
#include "../../devices/esp8266/esp8266_pdi.cpp"
#elif defined(DEVICE_ARDUINOUNO)
#include "../../devices/arduinouno/arduinouno_pdi.cpp"
#else
#include "../../devices/mockdevice/mockdevice_pdi.cpp"
#endif
