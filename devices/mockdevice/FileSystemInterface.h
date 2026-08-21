/*************************** File System Interface ****************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 16th Aug 2026
******************************************************************************/

#ifndef _MOCKDEVICE_FILE_SYSTEM_INTERFACE_H
#define _MOCKDEVICE_FILE_SYSTEM_INTERFACE_H

#include "StorageInterface.h"
#include <interface/pdi/impl/modules/storage/FileSystemInterfaceImpl.h>
#include <interface/pdi/impl/modules/storage/VfsDispatcher.h>

/**
 * @class FileSystemInterface
 *
 * Root mount backend, running the real LittleFS over the emulated flash.
 */
class FileSystemInterface : public FileSystemInterfaceImpl
{

public:
    FileSystemInterface() : FileSystemInterfaceImpl(__i_storage, false)
    {
    }

    virtual ~FileSystemInterface()
    {
    }
};

extern FileSystemInterface __i_rootfs;

#endif // _MOCKDEVICE_FILE_SYSTEM_INTERFACE_H
