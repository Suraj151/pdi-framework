/***************************** Instance Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#include "InstanceInterface.h"
#include "DeviceControlInterface.h"
#ifdef ENABLE_NETWORK_SERVICE
#include "TcpServerInterface.h"
#include "TcpClientInterface.h"
#include "UdpInterface.h"
#endif
#ifdef ENABLE_TLS_SERVICE
#include "TlsServerInterface.h"
#include "TlsClientInterface.h"
#endif
#ifdef ENABLE_STORAGE_SERVICE
#include "FileSystemInterface.h"
#endif

/**
 * InstanceInterface constructor.
 */
InstanceInterface::InstanceInterface(){
}

/**
 * InstanceInterface destructor.
 */
InstanceInterface::~InstanceInterface(){
}

iUtilityInterface &InstanceInterface::getUtilityInstance()
{
    return __i_dvc_ctrl;
}

#ifdef ENABLE_NETWORK_SERVICE
iTcpServerInterface *InstanceInterface::getNewTcpServerInstance()
{
    return new TcpServerInterface();
}

iTcpClientInterface *InstanceInterface::getNewTcpClientInstance()
{
    return new TcpClientInterface();
}

iUdpInterface *InstanceInterface::getNewUdpInstance()
{
    return new UdpInterface();
}
#endif

#ifdef ENABLE_TLS_SERVICE
iTlsServerInterface *InstanceInterface::getNewTlsServerInstance()
{
    return new TlsServerInterface();
}

iTlsClientInterface *InstanceInterface::getNewTlsClientInstance()
{
    return new TlsClientInterface();
}
#endif

#ifdef ENABLE_STORAGE_SERVICE
iFileSystemInterface &InstanceInterface::getFileSystemInstance()
{
    return __i_fs;
}
#endif

InstanceInterface __i_instance;
