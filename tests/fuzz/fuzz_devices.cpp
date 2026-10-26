/****************************** Fuzz Devices **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The declaration reader behind /etc/devices.conf: one line split into an
interface, a target and a driver name, then named the way /sys/bus names it.
The file is read again whenever it changes, so a line that got there by an
upload is read the same as one that was typed.

Author          : Suraj I.
created Date    : 19th Oct 2026
******************************************************************************/

#include <FuzzCommon.h>

#ifdef ENABLE_DRIVER_CORE

#include <service_provider/transport/DeviceManager.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    pdifuzz::FuzzInput input(data, size);

    uint16_t len = (uint16_t)((input.remaining() > 0xFFFF) ? 0xFFFF : input.remaining());
    pdiutil::string line((const char *)input.rest(), len);

    driver_device_t dev;
    if (DeviceManager::parseLine(line.c_str(), (uint16_t)line.size(), dev))
    {
        pdiutil::string id;
        pdiutil::string bus;
        DeviceManager::deviceId(dev, id);
        DeviceManager::busName(dev.m_bus, bus);
    }

    return 0;
}

#else

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    return 0;
}

#endif
