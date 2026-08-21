/***************************** Instance Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include "InstanceInterface.h"
#include "DeviceControlInterface.h"
#ifdef ENABLE_NETWORK_SERVICE
#include "TcpClientInterface.h"
#include "TcpServerInterface.h"
#include "UdpInterface.h"
#endif
#ifdef ENABLE_STORAGE_SERVICE
#include "FileSystemInterface.h"
#endif

/**
 * InstanceInterface constructor.
 */
InstanceInterface::InstanceInterface()
{
}

/**
 * InstanceInterface destructor.
 */
InstanceInterface::~InstanceInterface()
{
}

iUtilityInterface &InstanceInterface::getUtilityInstance()
{
    return __i_dvc_ctrl;
}

#ifdef ENABLE_NETWORK_SERVICE
iTcpServerInterface *InstanceInterface::getNewTcpServerInstance()
{
    return pdiutil::safe_new<TcpServerInterface>();
}

iTcpClientInterface *InstanceInterface::getNewTcpClientInstance()
{
    return pdiutil::safe_new<TcpClientInterface>();
}

iUdpInterface *InstanceInterface::getNewUdpInstance()
{
    return pdiutil::safe_new<UdpInterface>();
}
#endif

#ifdef ENABLE_STORAGE_SERVICE
iFileSystemInterface &InstanceInterface::getFileSystemInstance()
{
    return __i_fs;
}
#endif

InstanceInterface __i_instance;
