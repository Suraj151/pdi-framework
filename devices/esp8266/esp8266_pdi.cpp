/****************** ESP8266 Portable Device Interface ************************
This file is part of the pdi stack.

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
#if defined(LOGBEGIN) && ( defined(ENABLE_LOG_ALL) || defined(ENABLE_LOG_INFO) || defined(ENABLE_LOG_ERROR) || defined(ENABLE_LOG_WARNING) || defined(ENABLE_LOG_SUCCESS) )
#include "LoggerInterface.cpp"
#endif
#include "DatabaseInterface.cpp"
#include "DeviceControlInterface.cpp"
#ifdef ENABLE_WIFI_SERVICE
#include "WiFiInterface.cpp"
#include "WiFiClientInterface.cpp"
#include "WiFiServerInterface.cpp"
#endif
#ifdef ENABLE_NETWORK_SERVICE
#include "NtpInterface.cpp"
#include "PingInterface.cpp"
#include "TcpClientInterface.cpp"
#endif
#include "ExceptionsNotifier.cpp"
#include "core/Espnow.cpp"
#include "core/EW_EEPROM.cpp"
#ifdef ENABLE_SERIAL_SERVICE
#include "SerialInterface.cpp"
#endif
#ifdef ENABLE_STORAGE_SERVICE
#include "StorageInterface.cpp"
#include "FileSystemInterface.cpp"
#endif

