/***************************** Upgrade Interface ******************************
This file is part of the pdi stack.

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

#ifdef ENABLE_OTA_SERVICE
  // upgrade api. Optional client is an opaque pointer the device port may cast
  // to a framework Http_Client when it needs to fetch bytes over HTTP.
  virtual upgrade_status_t Upgrade(const char *path, const char *version, void *client = nullptr) = 0;

  // flash a firmware image already present on the filesystem. ports without a
  // local update path leave the default, which reports the attempt as failed.
  virtual upgrade_status_t UpgradeFromFile(const char *path) { (void)path; return UPGRADE_STATUS_FAILED; }
#endif
};

#endif
