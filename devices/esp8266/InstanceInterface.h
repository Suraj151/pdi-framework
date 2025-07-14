/***************************** Instance Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _ESP8266_INSTANCE_INTERFACE_H_
#define _ESP8266_INSTANCE_INTERFACE_H_

#include "esp8266.h"
#include <interface/interface_includes.h>
/**
 * InstanceInterface class
 */
class InstanceInterface: public iInstanceInterface
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

  iUtilityInterface& getUtilityInstance() override;
  iTcpServerInterface* getNewTcpServerInstance() override;
  iTcpClientInterface* getNewTcpClientInstance() override;
  iFileSystemInterface& getFileSystemInstance() override;
};

#endif
