/********************** Mock Portable Device Interface ************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _MOCKDEVICE_PORTABLE_DEVICE_INTERFACE_H_
#define _MOCKDEVICE_PORTABLE_DEVICE_INTERFACE_H_

#include "DatabaseInterface.h"
#include "DeviceControlInterface.h"
#ifdef ENABLE_WIFI_SERVICE
#include "WiFiInterface.h"
#include "HttpServerInterface.h"
#endif
// the storage layer stamps file times through the ntp interface, so it comes
// along whenever either service is on
#if defined(ENABLE_NETWORK_SERVICE) || defined(ENABLE_STORAGE_SERVICE)
#include "NtpInterface.h"
#endif
#ifdef ENABLE_NETWORK_SERVICE
#include "PingInterface.h"
#include "TcpClientInterface.h"
#include "TcpServerInterface.h"
#include "UdpInterface.h"
#endif
#ifdef ENABLE_SERIAL_SERVICE
#include "SerialInterface.h"
#endif
#ifdef ENABLE_STORAGE_SERVICE
#include "StorageInterface.h"
#include "FileSystemInterface.h"
#endif
#include "InstanceInterface.h"

#endif  // _MOCKDEVICE_PORTABLE_DEVICE_INTERFACE_H_
