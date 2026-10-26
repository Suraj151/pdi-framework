/****************************** Driver Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

What the serial service asks of a driver once a device it matches turns up: to
confirm the device really is the one it drives, and to let go of it when the
device goes away.

Author          : Suraj I.
created Date    : 19th Oct 2026
******************************************************************************/

#ifndef _I_DRIVER_INTERFACE_H_
#define _I_DRIVER_INTERFACE_H_

#include <config/Config.h>
#include <config/DriverConfig.h>
#include <interface/pdi/modules/serial/iSerialInterface.h>

/**
 * iDriverInterface class
 */
class iDriverInterface
{
public:

  /**
   * iDriverInterface destructor.
   */
  virtual ~iDriverInterface() {}

  /**
   * Confirms the device is the one this driver drives and takes it; PDI_OK, or an
   * error for anything else.
   */
  virtual pdi_err_t probe(driver_device_t &dev, iSerialInterface *bus) = 0;

  /**
   * Lets go of a device that has gone away or is being unbound.
   */
  virtual void remove(driver_device_t &dev) {}

  /**
   * What kind of device the driver presents, so a caller holding the base can
   * tell which interface it may treat it as.
   */
  virtual uint8_t driverClass() const { return DRIVER_CLASS_NONE; }

  /**
   * The path under /dev the device is reached at, false when it has none.
   */
  virtual bool nodeName(pdiutil::string &out) const { return false; }

  /**
   * Bytes read from the device through its /dev node.
   */
  virtual int32_t readNode(uint8_t *buf, uint32_t size) { return PDI_ERR_NOT_SUPPORTED; }

  /**
   * Whether the driver is mid operation on its device, so a scan neither takes its
   * silence as removal nor lets it go.
   */
  virtual bool busy() const { return false; }

  /**
   * Whether the device still answers, asked quickly on each scan of a driver
   * registered to be rescanned.
   */
  virtual bool stillThere() { return true; }
};

#endif
