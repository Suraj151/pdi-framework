/*************************** Nvmem Driver Interface ***************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

A driver that presents a memory which keeps its contents without power, read
and written at any offset through the device's eeprom file under /sys/bus.

Author          : Suraj I.
created Date    : 20th Oct 2026
******************************************************************************/

#ifndef _I_NVMEM_DRIVER_INTERFACE_H_
#define _I_NVMEM_DRIVER_INTERFACE_H_

#include "iDriverInterface.h"

/**
 * iNvmemDriverInterface class
 */
class iNvmemDriverInterface : public iDriverInterface
{
public:

  /**
   * iNvmemDriverInterface destructor.
   */
  virtual ~iNvmemDriverInterface() {}

  /**
   * Bytes the memory holds.
   */
  virtual uint32_t size() const = 0;

  /**
   * Bytes read from the offset up to the end of the memory: how many, 0 at or past
   * the end, or an error code.
   */
  virtual int32_t readAt(uint32_t offset, uint8_t *buf, uint32_t len) = 0;

  /**
   * Bytes stored from the offset, cut at the end of the memory: how many were
   * stored, or the code that names what failed.
   */
  virtual int32_t writeAt(uint32_t offset, const uint8_t *buf, uint32_t len) = 0;

  /**
   * A memory driver presents a memory.
   */
  uint8_t driverClass() const override { return DRIVER_CLASS_NVMEM; }
};

#endif
