/****************************** Serial Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _SERIAL_INTERFACE_H_
#define _SERIAL_INTERFACE_H_

#include "esp32.h"
#include <interface/pdi/modules/serial/iSerialInterface.h>

/**
 * UARTSerial class
 */
class UARTSerial : public iSerialInterface
{

public:
  /**
   * UARTSerial constructor.
   */
  UARTSerial(HardwareSerial& hwserial);
#if ARDUINO_USB_CDC_ON_BOOT
  /**
   * UARTSerial constructor for USB CDC backed Serial (S2/S3/C3/C6/H2 with CDC on boot).
   */
  UARTSerial(HWCDC& cdcserial);
#endif
  /**
   * UARTSerial destructor.
   */
  ~UARTSerial();

  // connect/disconnect api
  int16_t connect(uint16_t port, uint64_t speed) override;
  int16_t disconnect() override;

  // data sending api
  int32_t write(uint8_t c) override;
  int32_t write(const uint8_t *c_str) override;
  int32_t write(const uint8_t *c_str, uint32_t size) override;
  int32_t write_ro(const char *c_str) override;

  // received data read api
  uint8_t read() override;
  int32_t read(uint8_t *buf, uint32_t size) override;

  // useful api
  int32_t available() override;
  int8_t connected() override;
  void setTimeout(uint32_t timeout) override;
  void flush(int16_t flushtype = FLUSH_TX) override;

  // terminal api
  iTerminalInterface* with_timestamp() override;

private:
  bool m_connected;
  uint16_t m_port;
  uint64_t m_speed;
  HardwareSerial& m_hwserial;
#if ARDUINO_USB_CDC_ON_BOOT
  HWCDC* m_cdcserial;
#endif
};

extern UARTSerial __serial_uart;
extern UARTSerial __serial_uart1;

#ifdef ENABLE_I2S_BUS

#include <interface/pdi/modules/serial/iI2sSerialInterface.h>

class I2SClass;

/**
 * I2sSerial class
 *
 * Captures from an I2S microphone, holding the peripheral only while the bus is
 * configured.
 */
class I2sSerial : public iI2sSerialInterface
{
public:
  /**
   * I2sSerial constructor.
   */
  I2sSerial();

  /**
   * I2sSerial destructor.
   */
  ~I2sSerial();

  pdi_err_t configure(const serial_i2s_config_t &config) override;
  void release() override;
  int32_t write(const uint8_t *c_str, uint32_t size) override;
  int32_t read(uint8_t *buf, uint32_t size) override;
  uint8_t defaultPins(uint8_t *out, uint8_t max) const override;

private:
  I2SClass *m_i2s;
  int32_t *m_raw;
  int8_t m_bclk;
  int8_t m_ws;
  int8_t m_din;
};

extern I2sSerial __serial_i2s;

#endif

#ifdef ENABLE_I2C_BUS

#include <interface/pdi/modules/serial/iI2cSerialInterface.h>

/**
 * I2cSerial class
 *
 * The first I2C peripheral, started the first time it is used.
 */
class I2cSerial : public iI2cSerialInterface
{
public:
  /**
   * I2cSerial constructor.
   */
  I2cSerial();

  bool probe(uint32_t target) override;
  int32_t transfer(const uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen) override;
  int32_t transferTo(uint32_t target, const uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen) override;
  uint8_t defaultPins(uint8_t *out, uint8_t max) const override;
  pdi_err_t configure(int16_t sda, int16_t scl, uint32_t clock) override;

private:
  /**
   * Starts the bus unless it already runs.
   */
  pdi_err_t start();

  /**
   * The framework code for a failed transaction, asking the address again to tell a
   * refused address from refused data.
   */
  pdi_err_t failure(int32_t err, uint32_t target);
};

extern I2cSerial __serial_i2c;

#endif

#ifdef ENABLE_SPI_BUS

#include <interface/pdi/modules/serial/iSpiSerialInterface.h>

class SPIClass;

/**
 * SpiSerial class
 *
 * A general purpose SPI peripheral; the chip select is driven as a plain output.
 */
class SpiSerial : public iSpiSerialInterface
{
public:
  /**
   * SpiSerial constructor.
   */
  SpiSerial();

  /**
   * SpiSerial destructor.
   */
  ~SpiSerial();

  pdi_err_t begin(int16_t cs, uint32_t clock, uint8_t mode) override;
  void end() override;
  int32_t exchange(const uint8_t *tx, uint8_t *rx, uint32_t len) override;
  uint8_t defaultPins(uint8_t *out, uint8_t max) const override;
  pdi_err_t configure(int16_t sck, int16_t miso, int16_t mosi, uint32_t clock) override;

private:
  /**
   * Starts the bus unless it already runs.
   */
  pdi_err_t start();

  SPIClass *m_spi;
  bool m_started;
  bool m_open;
  int16_t m_cs;
};

extern SpiSerial __serial_spi;

#endif


#endif
