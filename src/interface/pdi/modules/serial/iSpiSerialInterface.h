/**************************** SPI Serial Interface ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

An SPI bus as a serial interface. A device is talked to inside a transaction:
begin() takes the bus at the clock and mode the device needs and pulls its
chip select low, exchange() clocks bytes out and in at once, and end() lets
the chip select go and hands the bus back. Every stream call is answered from
exchange() inside the transaction, so whatever works on a serial interface
works on the bus too.

Author          : Suraj I.
created Date    : 20th Oct 2026
******************************************************************************/

#ifndef _I_SPI_SERIAL_INTERFACE_H_
#define _I_SPI_SERIAL_INTERFACE_H_

#include "iSerialInterface.h"
#include <config/DriverConfig.h>

/**
 * iSpiSerialInterface class
 */
class iSpiSerialInterface : public iSerialInterface
{
public:

  /**
   * iSpiSerialInterface constructor.
   */
  iSpiSerialInterface() : m_sck(-1), m_miso(-1), m_mosi(-1), m_clock(0) {}

  /**
   * iSpiSerialInterface destructor.
   */
  virtual ~iSpiSerialInterface() {}

  using iSerialInterface::write;
  using iSerialInterface::read;

  /**
   * Puts the bus on the pins and top clock given, -1 and 0 for the port's own;
   * PDI_OK, or why the port cannot.
   */
  virtual pdi_err_t configure(int16_t sck, int16_t miso, int16_t mosi, uint32_t clock) {
    m_sck = sck;
    m_miso = miso;
    m_mosi = mosi;
    m_clock = clock;
    return PDI_OK;
  }

  /**
   * Takes the bus for one device at the clock and mode given and pulls its chip
   * select low, -1 for none, until end().
   */
  virtual pdi_err_t begin(int16_t cs, uint32_t clock, uint8_t mode) = 0;

  /**
   * Lets the chip select go high and hands the bus back.
   */
  virtual void end() = 0;

  /**
   * Clocks len bytes out of tx and into rx at once, 0xFF out for a null tx and
   * input dropped for a null rx; how many.
   */
  virtual int32_t exchange(const uint8_t *tx, uint8_t *rx, uint32_t len) = 0;

  /**
   * The clock pin the bus runs on, -1 when there is none to give.
   */
  int16_t sck() const { return pin(0, m_sck); }

  /**
   * The data in pin the bus runs on, -1 when there is none to give.
   */
  int16_t miso() const { return pin(1, m_miso); }

  /**
   * The data out pin the bus runs on, -1 when there is none to give.
   */
  int16_t mosi() const { return pin(2, m_mosi); }

  /**
   * The top clock the bus runs a device at.
   */
  uint32_t clock() const { return (0 != m_clock) ? m_clock : (uint32_t)SPI_BUS_CLOCK_HZ; }

  /**
   * The clock a device asking for the one given gets: no more than the bus's
   * top clock.
   */
  uint32_t clockFor(uint32_t asked) const { return (asked < clock()) ? asked : clock(); }

  /**
   * A bus holds no connection of its own to drop.
   */
  int16_t disconnect() override { return PDI_OK; }

  /**
   * One byte clocked out inside the transaction.
   */
  int32_t write(uint8_t c) override { return exchange(&c, nullptr, 1); }

  /**
   * A terminated string clocked out inside the transaction.
   */
  int32_t write(const uint8_t *c_str) override {
    return (nullptr == c_str) ? 0 : exchange(c_str, nullptr, strlen((const char *)c_str));
  }

  /**
   * Bytes clocked out inside the transaction.
   */
  int32_t write(const uint8_t *c_str, uint32_t size) override {
    return (nullptr == c_str) ? 0 : exchange(c_str, nullptr, size);
  }

  /**
   * One byte clocked in inside the transaction.
   */
  uint8_t read() override {
    uint8_t c = 0xFF;
    exchange(nullptr, &c, 1);
    return c;
  }

  /**
   * Bytes clocked in inside the transaction.
   */
  int32_t read(uint8_t *buf, uint32_t size) override {
    return (nullptr == buf) ? 0 : exchange(nullptr, buf, size);
  }

  /**
   * A device on the bus only answers while clocked, so nothing waits to be read.
   */
  int32_t available() override { return 0; }

  /**
   * A bus is there for as long as the port that made it.
   */
  int8_t connected() override { return 1; }

  /**
   * A transaction has no timeout of its own; a driver bounds its waits.
   */
  void setTimeout(uint32_t timeout) override {}

protected:

  /**
   * The pin configured, or the port's default at that place.
   */
  int16_t pin(uint8_t index, int16_t configured) const {
    if( configured >= 0 ){
      return configured;
    }
    uint8_t defaults[3] = {0, 0, 0};
    return (defaultPins(defaults, 3) > index) ? (int16_t)defaults[index] : (int16_t)-1;
  }

  int16_t m_sck;
  int16_t m_miso;
  int16_t m_mosi;
  uint32_t m_clock;
};

#endif
