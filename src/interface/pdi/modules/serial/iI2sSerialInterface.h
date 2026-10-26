/**************************** I2S Serial Interface ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

An I2S bus as a serial interface. Once configured, reading gives 16 bit PCM
samples as bytes and writing plays them, so whatever works on a serial
interface works on the bus too.

Author          : Suraj I.
created Date    : 19th Oct 2026
******************************************************************************/

#ifndef _I_I2S_SERIAL_INTERFACE_H_
#define _I_I2S_SERIAL_INTERFACE_H_

#include "iSerialInterface.h"

/**
 * iI2sSerialInterface class
 */
class iI2sSerialInterface : public iSerialInterface
{
public:

  /**
   * iI2sSerialInterface constructor.
   */
  iI2sSerialInterface() : m_timeout(0) {}

  /**
   * iI2sSerialInterface destructor.
   */
  virtual ~iI2sSerialInterface() {}

  using iSerialInterface::write;
  using iSerialInterface::read;

  /**
   * Wires and clocks the bus for capture or playback; PDI_OK, or the code that
   * names what stopped it.
   */
  virtual pdi_err_t configure(const serial_i2s_config_t &config) = 0;

  /**
   * Stops the bus and frees what configuring it took.
   */
  virtual void release() = 0;

  /**
   * Releasing the bus is all a disconnect means for it.
   */
  int16_t disconnect() override {
    release();
    return PDI_OK;
  }

  /**
   * One byte of PCM played.
   */
  int32_t write(uint8_t c) override { return write(&c, 1); }

  /**
   * A terminated run of PCM bytes played.
   */
  int32_t write(const uint8_t *c_str) override {
    return (nullptr == c_str) ? 0 : write(c_str, strlen((const char *)c_str));
  }

  /**
   * One byte of PCM captured.
   */
  uint8_t read() override {
    uint8_t c = 0;
    read(&c, 1);
    return c;
  }

  /**
   * The bus streams for as long as it is configured, so nothing is counted
   * as waiting.
   */
  int32_t available() override { return 0; }

  /**
   * A bus is there for as long as the port that made it.
   */
  int8_t connected() override { return 1; }

  /**
   * How long a read may wait for samples before it is given up.
   */
  void setTimeout(uint32_t timeout) override { m_timeout = timeout; }

protected:

  uint32_t m_timeout;
};

#endif
