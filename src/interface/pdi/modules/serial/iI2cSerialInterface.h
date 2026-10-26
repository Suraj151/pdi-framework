/**************************** I2C Serial Interface ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

An I2C bus as a serial interface. A port supplies the bus primitives, probe,
select and transfer, and every stream call is answered from them, so whatever
works on a serial interface works on the bus too.

Author          : Suraj I.
created Date    : 19th Oct 2026
******************************************************************************/

#ifndef _I_I2C_SERIAL_INTERFACE_H_
#define _I_I2C_SERIAL_INTERFACE_H_

#include "iSerialInterface.h"
#include <config/DriverConfig.h>

/**
 * iI2cSerialInterface class
 */
class iI2cSerialInterface : public iSerialInterface
{
public:

  /**
   * iI2cSerialInterface constructor.
   */
  iI2cSerialInterface() : m_selected(0), m_timeout(0), m_sda(-1), m_scl(-1), m_clock(0) {}

  /**
   * iI2cSerialInterface destructor.
   */
  virtual ~iI2cSerialInterface() {}

  /**
   * Puts the bus on the pins and clock given, -1 and 0 for the port's own; PDI_OK,
   * or why the port cannot.
   */
  virtual pdi_err_t configure(int16_t sda, int16_t scl, uint32_t clock) {
    m_sda = sda;
    m_scl = scl;
    m_clock = clock;
    return PDI_OK;
  }

  /**
   * The data pin the bus runs on, -1 when there is none to give.
   */
  int16_t sda() const { return pin(0, m_sda); }

  /**
   * The clock pin the bus runs on, -1 when there is none to give.
   */
  int16_t scl() const { return pin(1, m_scl); }

  /**
   * The clock the bus runs at.
   */
  uint32_t clock() const { return (0 != m_clock) ? m_clock : (uint32_t)I2C_BUS_CLOCK_HZ; }

  /**
   * Picks the 7 bit address the next read, write or transfer goes to.
   */
  int16_t select(uint32_t target) override {
    m_selected = target;
    return PDI_OK;
  }

  /**
   * The address the next read, write or transfer goes to.
   */
  uint32_t selected() const { return m_selected; }

  /**
   * One bounded exchange with the address, in one call so no other task moves the
   * bus to another address between.
   */
  virtual int32_t transferTo(uint32_t target, const uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen) {
    select(target);
    return transfer(tx, txlen, rx, rxlen);
  }

  /**
   * Writes the register pointer then the bytes to the selected address.
   */
  int32_t writeRegister(uint8_t reg, const uint8_t *data, uint32_t len) {
    uint8_t *frame = pdiutil::safe_new_array<uint8_t>(len + 1);
    if( nullptr == frame ){
      return PDI_ERR_NO_MEM;
    }
    frame[0] = reg;
    for( uint32_t i = 0; i < len && nullptr != data; i++ ){
      frame[i + 1] = data[i];
    }
    int32_t sent = transfer(frame, len + 1, nullptr, 0);
    pdiutil::safe_delete_array(frame);
    return sent;
  }

  /**
   * Writes the register pointer to the selected address and reads the bytes
   * that follow it.
   */
  int32_t readRegister(uint8_t reg, uint8_t *data, uint32_t len) {
    return transfer(&reg, 1, data, len);
  }

  /**
   * A bus holds no connection of its own to drop.
   */
  int16_t disconnect() override { return PDI_OK; }

  /**
   * One byte written to the selected address.
   */
  int32_t write(uint8_t c) override { return transfer(&c, 1, nullptr, 0); }

  /**
   * A terminated string written to the selected address.
   */
  int32_t write(const uint8_t *c_str) override {
    return (nullptr == c_str) ? 0 : transfer(c_str, strlen((const char *)c_str), nullptr, 0);
  }

  /**
   * The bytes written to the selected address in one transaction.
   */
  int32_t write(const uint8_t *c_str, uint32_t size) override {
    return (nullptr == c_str) ? 0 : transfer(c_str, size, nullptr, 0);
  }

  /**
   * One byte read from the selected address.
   */
  uint8_t read() override {
    uint8_t c = 0;
    transfer(nullptr, 0, &c, 1);
    return c;
  }

  /**
   * The bytes read from the selected address in one transaction.
   */
  int32_t read(uint8_t *buf, uint32_t size) override {
    return (nullptr == buf) ? 0 : transfer(nullptr, 0, buf, size);
  }

  /**
   * A device on the bus only answers when asked, so nothing waits to be read.
   */
  int32_t available() override { return 0; }

  /**
   * A bus is there for as long as the port that made it.
   */
  int8_t connected() override { return 1; }

  /**
   * How long a transaction may take before it is given up.
   */
  void setTimeout(uint32_t timeout) override { m_timeout = timeout; }

protected:

  /**
   * The pin configured, or the port's default at that place.
   */
  int16_t pin(uint8_t index, int16_t configured) const {
    if( configured >= 0 ){
      return configured;
    }
    uint8_t defaults[2] = {0, 0};
    return (defaultPins(defaults, 2) > index) ? (int16_t)defaults[index] : (int16_t)-1;
  }

  uint32_t m_selected;
  uint32_t m_timeout;
  int16_t m_sda;
  int16_t m_scl;
  uint32_t m_clock;
};

#endif
