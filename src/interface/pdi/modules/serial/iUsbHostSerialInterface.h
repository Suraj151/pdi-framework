/************************** USB Host Serial Interface *************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

A usb host port as a serial interface. A port reports the devices attached to
it and moves bytes to and from the selected one, and every stream call is
answered from that, so whatever works on a serial interface works here too.

Author          : Suraj I.
created Date    : 19th Oct 2026
******************************************************************************/

#ifndef _I_USB_HOST_SERIAL_INTERFACE_H_
#define _I_USB_HOST_SERIAL_INTERFACE_H_

#include "iSerialInterface.h"

/**
 * iUsbHostSerialInterface class
 */
class iUsbHostSerialInterface : public iSerialInterface
{
public:

  /**
   * iUsbHostSerialInterface constructor.
   */
  iUsbHostSerialInterface() : m_selected(0), m_timeout(0) {}

  /**
   * iUsbHostSerialInterface destructor.
   */
  virtual ~iUsbHostSerialInterface() {}

  /**
   * How many devices are attached to the port right now.
   */
  virtual uint8_t deviceCount() = 0;

  /**
   * The descriptors of one attached device, false when there is no such one.
   */
  virtual bool deviceAt(uint8_t index, usb_device_info_t &out) = 0;

  /**
   * Whether a device with this address is attached.
   */
  bool probe(uint32_t target) override {
    usb_device_info_t info;
    for( uint8_t i = 0; i < deviceCount(); i++ ){
      if( deviceAt(i, info) && target == info.m_address ){
        return true;
      }
    }
    return false;
  }

  /**
   * Picks the device and endpoint the next transfer goes to, the address in the
   * upper byte and the endpoint in the lower.
   */
  int16_t select(uint32_t target) override {
    m_selected = target;
    return PDI_OK;
  }

  /**
   * The device and endpoint the next read, write or transfer goes to.
   */
  uint32_t selected() const { return m_selected; }

  /**
   * A port holds no connection of its own to drop.
   */
  int16_t disconnect() override { return PDI_OK; }

  /**
   * One byte sent to the selected endpoint.
   */
  int32_t write(uint8_t c) override { return transfer(&c, 1, nullptr, 0); }

  /**
   * A terminated string sent to the selected endpoint.
   */
  int32_t write(const uint8_t *c_str) override {
    return (nullptr == c_str) ? 0 : transfer(c_str, strlen((const char *)c_str), nullptr, 0);
  }

  /**
   * The bytes sent to the selected endpoint in one transfer.
   */
  int32_t write(const uint8_t *c_str, uint32_t size) override {
    return (nullptr == c_str) ? 0 : transfer(c_str, size, nullptr, 0);
  }

  /**
   * One byte taken from the selected endpoint.
   */
  uint8_t read() override {
    uint8_t c = 0;
    transfer(nullptr, 0, &c, 1);
    return c;
  }

  /**
   * The bytes taken from the selected endpoint in one transfer.
   */
  int32_t read(uint8_t *buf, uint32_t size) override {
    return (nullptr == buf) ? 0 : transfer(nullptr, 0, buf, size);
  }

  /**
   * Nothing is buffered until a transfer asks for it.
   */
  int32_t available() override { return 0; }

  /**
   * A port is there for as long as the board that made it.
   */
  int8_t connected() override { return 1; }

  /**
   * How long a transfer may take before it is given up.
   */
  void setTimeout(uint32_t timeout) override { m_timeout = timeout; }

protected:

  uint32_t m_selected;
  uint32_t m_timeout;
};

#endif
