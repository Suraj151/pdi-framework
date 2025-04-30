/******************** ARDUINO UNO Portable Device Interface *******************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#include "arduinouno_pdi.h"

/*
 * Since arduino platform ide only considers the files inside "src" dir of root folder structure for compilation
 * So, here we are importing cpp source files which are part of this portable device interface (pdi) and which
 * needs to be compiled
 */
#include "LoggerInterface.cpp"
#include "DatabaseInterface.cpp"
#include "DeviceControlInterface.cpp"
#ifdef ENABLE_SERIAL_SERVICE
#include "SerialInterface.cpp"
#endif
#ifdef ENABLE_STORAGE_SERVICE
#include "StorageInterface.cpp"
#include "FileSystemInterface.cpp"
#endif

