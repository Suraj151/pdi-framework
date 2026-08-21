/***************************** Instance Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#ifndef _MOCKDEVICE_INSTANCE_INTERFACE_H_
#define _MOCKDEVICE_INSTANCE_INTERFACE_H_

#include "mockdevice.h"
#include <interface/interface_includes.h>

/**
 * InstanceInterface class
 */
class InstanceInterface : public iInstanceInterface
{

public:
  /**
   * InstanceInterface constructor.
   */
  InstanceInterface();

  /**
   * InstanceInterface destructor.
   */
  ~InstanceInterface();

  iUtilityInterface &getUtilityInstance() override;

#ifdef ENABLE_NETWORK_SERVICE
  iTcpServerInterface *getNewTcpServerInstance() override;
  iTcpClientInterface *getNewTcpClientInstance() override;
  iUdpInterface *getNewUdpInstance() override;
#endif

#ifdef ENABLE_STORAGE_SERVICE
  iFileSystemInterface &getFileSystemInstance() override;
#endif
};

#endif // _MOCKDEVICE_INSTANCE_INTERFACE_H_
