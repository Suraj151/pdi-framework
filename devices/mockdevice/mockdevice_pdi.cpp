/************************* Mock device Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/
#include <config/Config.h>

#ifdef MOCK_DEVICE_TEST

#include "mockdevice_pdi.h"

/*
 * Since arduino platform ide only considers the files inside "src" dir of root folder structure for compilation
 * So, here we are importing cpp source files which are part of this portable device interface (pdi) and which
 * needs to be compiled
 */
#include "DatabaseInterface.cpp"
#include "DeviceControlInterface.cpp"
#ifdef ENABLE_WIFI_SERVICE
#include "WiFiInterface.cpp"
#include "HttpServerInterface.cpp"
#endif
#if defined(ENABLE_NETWORK_SERVICE) || defined(ENABLE_STORAGE_SERVICE)
#include "NtpInterface.cpp"
#endif
#ifdef ENABLE_NETWORK_SERVICE
#include "PingInterface.cpp"
#include "TcpClientInterface.cpp"
#include "TcpServerInterface.cpp"
#include "UdpInterface.cpp"
#endif
#ifdef ENABLE_SERIAL_SERVICE
#include "SerialInterface.cpp"
#endif
#ifdef ENABLE_STORAGE_SERVICE
#include "StorageInterface.cpp"
#include "FileSystemInterface.cpp"
#endif
#include "InstanceInterface.cpp"

/**
 * the host addresses read only data like any other memory, so the wrapper hands
 * back a copy the caller owns and later releases
 */
char *rofn::to_charptr(const void *rostr)
{
    if (rostr == nullptr)
    {
        return nullptr;
    }

    const char *p = reinterpret_cast<const char *>(rostr);
    auto len = strlen(p);

    char *buff = pdiutil::safe_new_array<char>(len + 1);
    if (nullptr == buff)
    {
        return nullptr;
    }

    memcpy(buff, p, len);
    buff[len] = '\0';

    return buff;
}

#endif
