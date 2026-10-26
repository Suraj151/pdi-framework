/******************************* INMP441 Driver *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

An INMP441 I2S microphone, declared in /etc/devices.conf with the pins it is
wired to, or none for the bus's own: i2s0 rx inmp441 [sck=<pin> ws=<pin>
sd=<pin>] [rate=<hz>]. It carries
no id to read back, so it is confirmed by its samples changing; a data pin with
nothing on it reads flat.

Author          : Suraj I.
created Date    : 19th Oct 2026
******************************************************************************/

#ifndef _INMP441_DRIVER_H_
#define _INMP441_DRIVER_H_

#include <config/Config.h>

#ifdef ENABLE_I2S_BUS

#include <interface/pdi/modules/driver/iSoundDriverInterface.h>
#include <interface/pdi/modules/serial/iI2sSerialInterface.h>

/**
 * Inmp441Driver class
 */
class Inmp441Driver : public iSoundDriverInterface
{
public:

  /**
   * Inmp441Driver constructor.
   */
  Inmp441Driver();

  /**
   * Frees the bus when the driver goes.
   */
  ~Inmp441Driver();

  /**
   * Adds the driver to the ones the serial service binds with.
   */
  static void registerDriver();

  /**
   * Wires the bus to the declared pins and confirms the mic by its samples.
   */
  pdi_err_t probe(driver_device_t &dev, iSerialInterface *bus) override;

  /**
   * Stops the bus and frees it.
   */
  void remove(driver_device_t &dev) override;

  /**
   * The capture node under /dev.
   */
  bool nodeName(pdiutil::string &out) const override;

  /**
   * PCM bytes captured from the mic.
   */
  int32_t readNode(uint8_t *buf, uint32_t size) override;

  /**
   * Samples per second, as declared or the default.
   */
  uint32_t rate() const override;

private:

  iI2sSerialInterface *m_bus;
  uint32_t m_rate;
};

#endif

#endif
