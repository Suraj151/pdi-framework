/********************************* AT24 Driver *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

An I2C eeprom of the 24c family, declared in /etc/devices.conf with its size
and page in bytes: i2c0 <address> at24 [chip=<name>] size=<bytes> page=<bytes>.
An eeprom carries no id to read back, so it is confirmed by answering at its
address and giving back its first byte. A chip of up to 2 KB takes a one byte
word address and the bits above it in its bus address; a larger one takes two.

Author          : Suraj I.
created Date    : 20th Oct 2026
******************************************************************************/

#ifndef _AT24_DRIVER_H_
#define _AT24_DRIVER_H_

#include <config/Config.h>

#ifdef ENABLE_I2C_BUS

#include <interface/pdi/modules/driver/iNvmemDriverInterface.h>
#include <interface/pdi/modules/serial/iI2cSerialInterface.h>

/**
 * At24Driver class
 */
class At24Driver : public iNvmemDriverInterface
{
public:

  /**
   * At24Driver constructor.
   */
  At24Driver();

  /**
   * Adds the driver to the ones the serial service binds with.
   */
  static void registerDriver();

  /**
   * Takes the declared size and page and confirms the chip answers; PDI_OK, or the
   * code that names what is wrong.
   */
  pdi_err_t probe(driver_device_t &dev, iSerialInterface *bus) override;

  /**
   * Lets go of the bus.
   */
  void remove(driver_device_t &dev) override;

  /**
   * Whether a read or write is under way, during which the chip may answer nothing
   * while it stores a page.
   */
  bool busy() const override;

  /**
   * Bytes the chip holds.
   */
  uint32_t size() const override;

  /**
   * Bytes read from the offset up to the end of the chip, busy throughout and
   * yielding between chunks.
   */
  int32_t readAt(uint32_t offset, uint8_t *buf, uint32_t len) override;

  /**
   * Bytes stored from the offset up to the end of the chip, a page at a time, busy
   * throughout and yielding between pages.
   */
  int32_t writeAt(uint32_t offset, const uint8_t *buf, uint32_t len) override;

private:

  /**
   * Gives the bus address of the block holding the offset and puts its word address
   * in the frame; the bytes it took.
   */
  uint8_t address(uint32_t offset, uint8_t *frame, uint32_t &target) const;

  /**
   * Waits for the chip to answer again once it has stored a page, giving up after
   * the longest a page may take.
   */
  pdi_err_t waitStored(uint32_t target);

  iI2cSerialInterface *m_bus;
  uint32_t m_target;
  uint32_t m_size;
  uint32_t m_page;
  bool m_busy;
};

#endif

#endif
