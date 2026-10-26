/****************************** Serial Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _PDI_POSIX_SERIAL_INTERFACE_H_
#define _PDI_POSIX_SERIAL_INTERFACE_H_

#include "posix.h"
#include <interface/pdi/modules/serial/iSerialInterface.h>
#include <interface/pdi/modules/serial/iI2cSerialInterface.h>
#include <interface/pdi/modules/serial/iUsbHostSerialInterface.h>
#include <interface/pdi/modules/serial/iI2sSerialInterface.h>

/**
 * UARTSerial class
 *
 * Carries the terminal over a pair of host file descriptors. Defaults to stdin
 * and stdout so the process is directly interactive, and accepts any other pair
 * so a caller can drive it from a pipe, socket or pty.
 */
class UARTSerial : public iSerialInterface
{

public:
  /**
   * UARTSerial constructor.
   */
  UARTSerial(int readfd, int writefd);

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

  /**
   * @brief With timestamp will print timestamp first
   * derived class should implement this function to print timestamp
   * @param None
   * @return this
   */
  iTerminalInterface* with_timestamp() override;

  /**
   * @brief Point the terminal at another pair of descriptors. Closes nothing;
   *        ownership of the descriptors stays with the caller.
   */
  void setDescriptors(int readfd, int writefd);

private:
  bool m_connected;
  uint16_t m_port;
  uint64_t m_speed;
  uint32_t m_timeout;
  int m_readfd;
  int m_writefd;
  int16_t m_peeked;

  bool fill_peek();
};

extern UARTSerial __serial_uart;
extern UARTSerial __serial_uart1;

/**
 * SimulatedBus class
 *
 * What is plugged into the simulated buses, read again on every ask from lines such
 * as `i2c0 0x50 42 01 02`.
 */
class SimulatedBus
{
public:
  /**
   * @brief Takes what is plugged in from this file from now on.
   */
  static void setInputFile(const char *path);

  /**
   * @brief Hands each line naming the bus to the visitor, until it answers false.
   */
  static void eachLine(const char *bus, bool (*visit)(const char *rest, void *arg), void *arg);

private:
  static const char *s_input_file;
};

/**
 * I2cSerial class
 *
 * An I2C bus whose chips are the lines of the simulated bus file.
 */
class I2cSerial : public iI2cSerialInterface
{
public:
  /**
   * @brief I2cSerial constructor, for the bus with this number.
   */
  explicit I2cSerial(uint8_t bus);

  bool probe(uint32_t target) override;
  int32_t transfer(const uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen) override;
  uint8_t defaultPins(uint8_t *out, uint8_t max) const override;

private:
  char m_name[5];
};

/**
 * UsbHostSerial class
 *
 * A usb host port whose devices are the lines of the simulated bus file.
 */
class UsbHostSerial : public iUsbHostSerialInterface
{
public:
  uint8_t deviceCount() override;
  bool deviceAt(uint8_t index, usb_device_info_t &out) override;
  int32_t transfer(const uint8_t *tx, uint32_t txlen, uint8_t *rx, uint32_t rxlen) override;
};

/**
 * I2sSerial class
 *
 * An I2S bus whose microphone is an `i2s0 noise` or `i2s0 silent` line of the
 * simulated bus file.
 */
class I2sSerial : public iI2sSerialInterface
{
public:
  /**
   * @brief I2sSerial constructor.
   */
  I2sSerial();

  pdi_err_t configure(const serial_i2s_config_t &config) override;
  void release() override;
  int32_t write(const uint8_t *c_str, uint32_t size) override;
  int32_t read(uint8_t *buf, uint32_t size) override;
  uint8_t defaultPins(uint8_t *out, uint8_t max) const override;

private:
  bool m_configured;
  uint32_t m_state;
};

#include <interface/pdi/modules/serial/iSpiSerialInterface.h>
#include <vector>

/**
 * SdCardSim class
 *
 * An SD card speaking SPI mode byte for byte from host memory, with switches for
 * the ways real cards misbehave.
 */
class SdCardSim
{
public:
  /**
   * @brief SdCardSim constructor, a card of the kind and blocks given.
   */
  SdCardSim(bool highcapacity, uint32_t blocks);

  /**
   * @brief SdCardSim destructor.
   */
  ~SdCardSim();

  /**
   * @brief The chip select going low or high; a card let go drops what it was half
   * saying or hearing.
   */
  void select(bool selected);

  /**
   * @brief One byte clocked in from the host, and the one clocked out to it.
   */
  uint8_t exchange(uint8_t in);

  /**
   * @brief The card's blocks.
   */
  uint8_t *data() { return m_store.data(); }

  /**
   * @brief Blocks the card holds.
   */
  uint32_t blocks() const { return m_blocks; }

  /**
   * The card put back in the slot, powered up afresh: idle, CRC off, needing to be
   * woken and reset again.
   */
  void reinsert();

  bool m_present;
  bool m_v1;
  bool m_mmc;
  bool m_no_crc_command;
  bool m_wrong_voltage;
  bool m_ultra_capacity;
  bool m_never_ready;
  int m_ready_after;
  uint8_t m_read_error;
  bool m_bad_crc;
  uint8_t m_write_response;
  int m_busy_bytes;
  bool m_never_done;
  int m_silent_command;
  int m_silent_count;
  int m_refused_command;
  bool m_bad_echo;
  bool m_no_token;
  bool m_no_write_response;
  int m_fail_read_at;
  int m_reads;
  int m_writes;
  int m_clocks_deselected;

private:
  enum SimState : uint8_t { SIM_COMMAND = 0, SIM_WRITE_TOKEN, SIM_WRITE_DATA, SIM_BUSY };

  void command();
  void queue(uint8_t b);
  void queueBlock(const uint8_t *block, uint32_t len, bool data);
  bool blockAt(uint32_t arg, uint32_t &offset) const;
  void csd(uint8_t *out) const;

  bool m_highcapacity;
  uint32_t m_blocks;
  std::vector<uint8_t> m_store;
  uint8_t *m_mem;
  bool m_selected;
  bool m_asleep;
  bool m_idle;
  bool m_app;
  bool m_crc_on;
  uint8_t m_state;
  uint8_t m_cmd[6];
  uint8_t m_cmdlen;
  uint8_t m_out[SD_BLOCK_SIZE + 24];
  uint32_t m_outhead;
  uint32_t m_outtail;
  uint8_t m_in[SD_BLOCK_SIZE + 2];
  uint32_t m_inlen;
  uint32_t m_write_offset;
  int m_busy_left;
};

/**
 * SpiSerial class
 *
 * An SPI bus whose cards are lines of the simulated bus file, such as `spi0 10 sd
 * hc 4096`.
 */
class SpiSerial : public iSpiSerialInterface
{
public:
  pdi_err_t begin(int16_t cs, uint32_t clock, uint8_t mode) override;
  void end() override;
  int32_t exchange(const uint8_t *tx, uint8_t *rx, uint32_t len) override;
  uint8_t defaultPins(uint8_t *out, uint8_t max) const override;

private:
  SdCardSim *card(int16_t cs);
  int16_t m_cs = -1;
  SdCardSim *m_card = nullptr;
};

extern I2cSerial __serial_i2c;
extern UsbHostSerial __serial_usb_host;
extern I2sSerial __serial_i2s;
extern SpiSerial __serial_spi;

#endif // _PDI_POSIX_SERIAL_INTERFACE_H_
