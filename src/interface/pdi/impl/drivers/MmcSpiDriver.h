/******************************* MMC SPI Driver *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

An SD card on the SPI bus, declared in /etc/devices.conf by the chip select it
is wired to: spi0 cs=<pin> mmc_spi. The driver speaks the card's SPI mode
itself: it wakes the card at a slow clock with its select high, resets it,
tells an older card from a newer one and a standard capacity card from a high
capacity one, reads its size, and from then on reads and writes it a 512 byte
block at a time at the fast clock. The card is presented as a disk.

Author          : Suraj I.
created Date    : 20th Oct 2026
******************************************************************************/

#ifndef _MMC_SPI_DRIVER_H_
#define _MMC_SPI_DRIVER_H_

#include <config/Config.h>

#ifdef ENABLE_SPI_BUS

#include <interface/pdi/modules/driver/iBlockDriverInterface.h>
#include <interface/pdi/modules/serial/iSpiSerialInterface.h>

/**
 * MmcSpiDriver class
 */
class MmcSpiDriver : public iBlockDriverInterface
{
public:

  /**
   * MmcSpiDriver constructor.
   */
  MmcSpiDriver();

  /**
   * Adds the driver to the ones the serial service binds with, rescanned so
   * a card taken out or put back is noticed.
   */
  static void registerDriver();

  /**
   * Wakes the card behind the declared chip select and reads its size.
   */
  pdi_err_t probe(driver_device_t &dev, iSerialInterface *bus) override;

  /**
   * Lets go of the bus.
   */
  void remove(driver_device_t &dev) override;

  /**
   * Whether a read or write is under way.
   */
  bool busy() const override;

  /**
   * Whether the card still answers a status command, asked a few times so one bus
   * glitch does not count as removal.
   */
  bool stillThere() override;

  /**
   * The disk's node under /dev.
   */
  bool nodeName(pdiutil::string &out) const override;

  /**
   * Blocks the card holds.
   */
  uint32_t blocks() const override;

  /**
   * Whole blocks read from the block given.
   */
  int32_t readBlocks(uint32_t block, uint8_t *buf, uint32_t count) override;

  /**
   * Whole blocks stored from the block given.
   */
  int32_t writeBlocks(uint32_t block, const uint8_t *buf, uint32_t count) override;

private:

  /**
   * Wakes, resets and identifies the card and reads its size; PDI_ERR_NOT_FOUND
   * when nothing answers the reset as SD.
   */
  pdi_err_t start();

  /**
   * Waits for the card to leave its idle state.
   */
  pdi_err_t waitReady(bool highcapacityasked);

  /**
   * Sends a command with its CRC and gives its first answer byte, -1 when nothing
   * answered, -2 when the card stayed busy.
   */
  int16_t command(uint8_t index, uint32_t arg);

  /**
   * Asks the card to check CRCs and send real ones with its data, carrying
   * on without data CRC checks when it does not know how.
   */
  pdi_err_t enableCrc();

  /**
   * Takes a data block the card sends, checking its CRC.
   */
  pdi_err_t receive(uint8_t *buf, uint32_t len);

  /**
   * Waits for the card to finish storing a block.
   */
  pdi_err_t waitStored();

  /**
   * Reads one block.
   */
  pdi_err_t readBlock(uint32_t block, uint8_t *buf);

  /**
   * Stores one block.
   */
  pdi_err_t writeBlock(uint32_t block, const uint8_t *buf);

  /**
   * Lets the chip select go and clocks once more so the card lets go of its
   * data line.
   */
  void finish();

  /**
   * The code for an answer byte that was not the one wanted.
   */
  pdi_err_t answerError(int16_t r1) const;

  /**
   * Where a block is, the way the card counts.
   */
  uint32_t address(uint32_t block) const;

  iSpiSerialInterface *m_bus;
  int16_t m_cs;
  uint32_t m_blocks;
  uint32_t m_clock;
  bool m_highcapacity;
  bool m_busy;
  bool m_crc;
};

#endif

#endif
