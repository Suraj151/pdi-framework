/***************************** Instance Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#include "InstanceInterface.h"
#include "DeviceControlInterface.h"
#include "TcpServerInterface.h"
#include "TcpClientInterface.h"
#ifdef ENABLE_TLS_SERVICE
#include "TlsServerInterface.h"
#include "TlsClientInterface.h"
#endif
#include "FileSystemInterface.h"

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

iTcpServerInterface *InstanceInterface::getNewTcpServerInstance()
{
    return new TcpServerInterface();
}

iTcpClientInterface *InstanceInterface::getNewTcpClientInstance()
{
    return new TcpClientInterface();
}

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

iFileSystemInterface &InstanceInterface::getFileSystemInstance()
{
    return __i_fs;
}

InstanceInterface __i_instance;
