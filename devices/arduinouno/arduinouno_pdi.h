/****************** Arduino Uno Portable Device Interface ********************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _ARDUINOUNO_PORTABLE_DEVICE_INTERFACE_H_
#define _ARDUINOUNO_PORTABLE_DEVICE_INTERFACE_H_

#include "LoggerInterface.h"
#include "DatabaseInterface.h"
#ifdef ENABLE_SERIAL_SERVICE
#include "SerialInterface.h"
#endif
#include "DeviceControlInterface.h"
#include "core/PDIEEPROM.h"
#ifdef ENABLE_STORAGE_SERVICE
#include "StorageInterface.h"
#include "FileSystemInterface.h"
#endif

#endif  // _ARDUINOUNO_PORTABLE_DEVICE_INTERFACE_H_
