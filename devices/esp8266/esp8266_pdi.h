/****************** ESP8266 Portable Device Interface ************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _ESP8266_PORTABLE_DEVICE_INTERFACE_H_
#define _ESP8266_PORTABLE_DEVICE_INTERFACE_H_

#if defined(LOGBEGIN) && ( defined(ENABLE_LOG_ALL) || defined(ENABLE_LOG_INFO) || defined(ENABLE_LOG_ERROR) || defined(ENABLE_LOG_WARNING) || defined(ENABLE_LOG_SUCCESS) )
#include "LoggerInterface.h"
#endif
#include "DatabaseInterface.h"
#include "DeviceControlInterface.h"
#ifdef ENABLE_WIFI_SERVICE
#include "WiFiInterface.h"
#include "WiFiClientInterface.h"
#include "HttpServerInterface.h"
#endif
#ifdef ENABLE_NETWORK_SERVICE
#include "NtpInterface.h"
#include "PingInterface.h"
#include "TcpClientInterface.h"
#include "TcpServerInterface.h"
#endif
#include "core/Espnow.h"
#ifdef ENABLE_SERIAL_SERVICE
#include "SerialInterface.h"
#endif
#ifdef ENABLE_STORAGE_SERVICE
#include "StorageInterface.h"
#include "FileSystemInterface.h"
#endif


#endif  // _ESP8266_PORTABLE_DEVICE_INTERFACE_H_
