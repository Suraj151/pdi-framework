/***************************** Upgrade Interface ******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_UPGRADE_INTERFACE_H_
#define _I_UPGRADE_INTERFACE_H_

#include <interface/interface_includes.h>

/**
 * iUpgradeInterface class
 */
class iUpgradeInterface
{

public:
  /**
   * iUpgradeInterface constructor.
   */
  iUpgradeInterface() {}
  /**
   * iUpgradeInterface destructor.
   */
  virtual ~iUpgradeInterface() {}

  // upgrade api
  virtual upgrade_status_t Upgrade(const char *path, const char *version) = 0;
};

#endif
