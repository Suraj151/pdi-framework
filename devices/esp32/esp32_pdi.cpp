/********************* ESP32 Portable Device Interface ************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#include "esp32_pdi.h"

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
#include "HttpServerInterface.cpp"
#endif
#ifdef ENABLE_NETWORK_SERVICE
#include "NtpInterface.cpp"
#include "PingInterface.cpp"
#include "TcpClientInterface.cpp"
#include "TcpServerInterface.cpp"
#endif
#include "ExceptionsNotifier.cpp"
#include "core/EEPROM.cpp"
#ifdef ENABLE_SERIAL_SERVICE
#include "SerialInterface.cpp"
#endif
#ifdef ENABLE_STORAGE_SERVICE
#include "StorageInterface.cpp"
#include "FileSystemInterface.cpp"
#endif
#include "InstanceInterface.cpp"

// This function converts a read-only string (PGM_P) to a dynamically allocated char pointer.
// The caller is responsible for deleting the allocated memory to avoid memory leaks.
// It reads the string from program memory and copies it to a new char array, which is
// then returned as a null-terminated string.
char* rofn::to_charptr(const void *rostr){
    if( rostr == nullptr ){
        return nullptr;
    }

    PGM_P p = reinterpret_cast<PGM_P>(rostr);
    auto len = strlen_P(p);

    char *buff = new char[len + 1]; // +1 for null terminator
    memcpy_P(buff, p, len);
    buff[len] = '\0'; // Null terminate the string

    // Return the pointer to the newly allocated string
    // Note: The caller is responsible for deleting this memory
    // to avoid memory leaks.
    return buff;
}
