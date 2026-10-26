/****************************** Serial Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _SERIAL_INTERFACE_H_
#define _SERIAL_INTERFACE_H_

#include "esp8266.h"
#include <interface/pdi/modules/serial/iSerialInterface.h>
#ifdef ENABLE_CONTEXTUAL_EXECUTION
#include "threading/PreemptiveMutex.h"
#endif

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

  // Direct hardware access for contexts that must not take a task owned lock.
  HardwareSerial& hw() { return m_hwserial; }

  // Make mutex to be accesible for log service for now.
  // todo : make this private once log service initializd with iointerface
  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  PreemptiveMutex m_mutex;
  #endif

private:
  bool m_connected;
  uint16_t m_port;
  uint64_t m_speed;
  HardwareSerial& m_hwserial;
};

extern UARTSerial __serial_uart;
extern UARTSerial __serial_uart1;

#ifdef ENABLE_I2S_BUS

#include <interface/pdi/modules/serial/iI2sSerialInterface.h>

/**
 * I2sSerial class
 *
 * Captures from an I2S microphone on the fixed pins, in 16 bit slots of 32 clocks a
 * frame.
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
  bool m_running;
};

extern I2sSerial __serial_i2s;

#endif

#ifdef ENABLE_I2C_BUS

#include <interface/pdi/modules/serial/iI2cSerialInterface.h>

/**
 * I2cSerial class
 *
 * The I2C bus on the port's pins, started the first time it is used.
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
   * Starts the bus on its pins and clock, unless it already runs.
   */
  void start();

  /**
   * One exchange with the address, with the bus already held.
   */
  int32_t exchange(uint32_t target, const uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen);

  bool m_started;

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  PreemptiveMutex m_mutex;
  #endif
};

extern I2cSerial __serial_i2c;

#endif

#ifdef ENABLE_SPI_BUS

#include <interface/pdi/modules/serial/iSpiSerialInterface.h>

/**
 * SpiSerial class
 *
 * The SPI peripheral on its fixed pins; the chip select is any free gpio, driven as
 * a plain output.
 */
class SpiSerial : public iSpiSerialInterface
{
public:
  /**
   * SpiSerial constructor.
   */
  SpiSerial();

  pdi_err_t begin(int16_t cs, uint32_t clock, uint8_t mode) override;
  void end() override;
  int32_t exchange(const uint8_t *tx, uint8_t *rx, uint32_t len) override;
  uint8_t defaultPins(uint8_t *out, uint8_t max) const override;
  pdi_err_t configure(int16_t sck, int16_t miso, int16_t mosi, uint32_t clock) override;

private:
  bool m_started;
  bool m_open;
  int16_t m_cs;

  #ifdef ENABLE_CONTEXTUAL_EXECUTION
  PreemptiveMutex m_mutex;
  #endif
};

extern SpiSerial __serial_spi;

#endif


#endif
