/****************** ESP8266 Portable Device Interface ************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#include "esp8266_pdi.h"

/*
 * Since arduino platform ide only considers the files inside "src" dir of root folder structure for compilation
 * So, here we are importing cpp source files which are part of this portable device interface (pdi) and which
 * needs to be compiled
 */
#include "LoggerInterface.cpp"
#include "DatabaseInterface.cpp"
#include "DeviceControlInterface.cpp"
#include "WiFiInterface.cpp"
#include "WiFiClientInterface.cpp"
#include "WiFiServerInterface.cpp"
#include "NtpInterface.cpp"
#include "PingInterface.cpp"
#include "ExceptionsNotifier.cpp"
#include "core/Espnow.cpp"
#include "core/EW_EEPROM.cpp"

