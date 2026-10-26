/******************************* MMC SPI Driver *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 20th Oct 2026
******************************************************************************/

#include "MmcSpiDriver.h"

#ifdef ENABLE_SPI_BUS

#include <service_provider/transport/DeviceManager.h>
#include <utility/Checksum.h>
#include <interface/pdi.h>

namespace {

const uint8_t s_sd_idle = 0x01;
const uint8_t s_sd_illegal = 0x04;
const uint8_t s_sd_data_token = 0xFE;
const uint8_t s_sd_accepted = 0x05;
const uint8_t s_sd_crc_rejected = 0x0B;
const uint8_t s_sd_answer_wait = 8;
const uint8_t s_sd_wake_bytes = 10;
const uint8_t s_sd_reset_tries = 5;
const uint8_t s_sd_alive_tries = 3;
const uint8_t s_sd_polls_per_yield = 32;
const uint32_t s_sd_v2_check = 0x1AA;
const uint32_t s_sd_high_capacity = 0x40000000UL;

/**
 * Makes a driver for a device that was declared.
 */
iDriverInterface *createMmcSpi()
{
  return pdiutil::safe_new<MmcSpiDriver>();
}

}

/**
 * MmcSpiDriver constructor.
 */
MmcSpiDriver::MmcSpiDriver() : m_bus(nullptr), m_cs(-1), m_blocks(0), m_clock(SD_INIT_CLOCK_HZ),
                               m_highcapacity(false), m_busy(false), m_crc(false)
{
}

/**
 * Adds the driver to the ones the serial service binds with, rescanned so
 * a card taken out or put back is noticed.
 */
void MmcSpiDriver::registerDriver()
{
  DeviceManager::registerDriver(driver_prop_t(RODT_ATTR(DRIVER_NAME_MMC_SPI), SERIAL_IFACE_SPI,
                                              nullptr, 0, nullptr, createMmcSpi, true));
}

/**
 * Wakes the card behind the declared chip select and reads its size.
 */
pdi_err_t MmcSpiDriver::probe(driver_device_t &dev, iSerialInterface *bus)
{
  if( nullptr == bus || SERIAL_IFACE_SPI != serialFamily(bus->type()) || dev.m_target > 0x7F ){
    return PDI_ERR_NOT_SUPPORTED;
  }

  m_bus = static_cast<iSpiSerialInterface *>(bus);
  m_cs = (int16_t)dev.m_target;
  m_clock = SD_INIT_CLOCK_HZ;

  pdi_err_t started = start();
  if( PDI_OK != started ){
    m_bus = nullptr;
    return started;
  }

  m_clock = SD_CLOCK_HZ;
  return PDI_OK;
}

/**
 * Lets go of the bus.
 */
void MmcSpiDriver::remove(driver_device_t &dev)
{
  m_bus = nullptr;
}

/**
 * Whether a read or write is under way.
 */
bool MmcSpiDriver::busy() const
{
  return m_busy;
}

/**
 * Whether the card still answers a status command, asked a few times so one bus
 * glitch does not count as removal.
 */
bool MmcSpiDriver::stillThere()
{
  if( nullptr == m_bus ){
    return false;
  }
  for( uint8_t i = 0; i < s_sd_alive_tries; i++ ){
    if( PDI_OK != m_bus->begin(m_cs, m_clock, 0) ){
      return true;
    }
    int16_t r1 = command(13, 0);
    if( r1 >= 0 ){
      uint8_t r2 = 0xFF;
      m_bus->exchange(nullptr, &r2, 1);
    }
    finish();
    if( -1 != r1 ){
      return true;
    }
  }
  return false;
}

/**
 * The disk's node under /dev.
 */
bool MmcSpiDriver::nodeName(pdiutil::string &out) const
{
  out = CHARPTR_WRAP(DEVICE_NODE_MMC_BLOCK);
  return true;
}

/**
 * Blocks the card holds.
 */
uint32_t MmcSpiDriver::blocks() const
{
  return m_blocks;
}

/**
 * Where a block is, the way the card counts.
 */
uint32_t MmcSpiDriver::address(uint32_t block) const
{
  return m_highcapacity ? block : block * SD_BLOCK_SIZE;
}

/**
 * The code for an answer byte that was not the one wanted.
 */
pdi_err_t MmcSpiDriver::answerError(int16_t r1) const
{
  if( -2 == r1 ){
    return PDI_ERR_TIMEOUT;
  }
  return (r1 < 0) ? PDI_ERR_NOT_FOUND : PDI_ERR_IO;
}

/**
 * Lets the chip select go and clocks once more so the card lets go of its
 * data line.
 */
void MmcSpiDriver::finish()
{
  m_bus->end();
  if( PDI_OK == m_bus->begin(-1, m_clock, 0) ){
    m_bus->exchange(nullptr, nullptr, 1);
    m_bus->end();
  }
}

/**
 * Sends a command with its CRC and gives its first answer byte, -1 when nothing
 * answered, -2 when the card stayed busy.
 */
int16_t MmcSpiDriver::command(uint8_t index, uint32_t arg)
{
  if( 0 != index && PDI_OK != waitStored() ){
    return -2;
  }

  uint8_t frame[6] = {
    (uint8_t)(0x40 | index),
    (uint8_t)(arg >> 24), (uint8_t)(arg >> 16), (uint8_t)(arg >> 8), (uint8_t)arg,
    0
  };
  frame[5] = (uint8_t)((crc7(frame, 5) << 1) | 0x01);
  m_bus->exchange(frame, nullptr, sizeof(frame));

  for( uint8_t i = 0; i < s_sd_answer_wait; i++ ){
    uint8_t r1 = 0xFF;
    m_bus->exchange(nullptr, &r1, 1);
    if( 0 == (r1 & 0x80) ){
      return r1;
    }
  }
  return -1;
}

/**
 * Wakes, resets and identifies the card and reads its size; PDI_ERR_NOT_FOUND when
 * nothing answers the reset as SD.
 */
pdi_err_t MmcSpiDriver::start()
{
  pdi_err_t opened = m_bus->begin(-1, m_clock, 0);
  if( PDI_OK != opened ){
    return opened;
  }
  m_bus->exchange(nullptr, nullptr, s_sd_wake_bytes);
  m_bus->end();

  opened = m_bus->begin(m_cs, m_clock, 0);
  if( PDI_OK != opened ){
    return opened;
  }

  int16_t r1 = -1;
  for( uint8_t i = 0; i < s_sd_reset_tries && s_sd_idle != r1; i++ ){
    r1 = command(0, 0);
  }
  if( s_sd_idle != r1 ){
    finish();
    return PDI_ERR_NOT_FOUND;
  }

  bool v2 = false;
  r1 = command(8, s_sd_v2_check);
  if( r1 < 0 ){
    finish();
    return answerError(r1);
  }
  if( 0 == (r1 & s_sd_illegal) ){
    uint8_t r7[4] = {0, 0, 0, 0};
    m_bus->exchange(nullptr, r7, sizeof(r7));
    if( (s_sd_v2_check & 0xFF) != r7[3] ){
      finish();
      return PDI_ERR_IO;
    }
    if( ((s_sd_v2_check >> 8) & 0x0F) != (r7[2] & 0x0F) ){
      finish();
      return PDI_ERR_NOT_SUPPORTED;
    }
    v2 = true;
  }

  pdi_err_t crc = enableCrc();
  finish();
  if( PDI_OK != crc ){
    return crc;
  }

  pdi_err_t ready = waitReady(v2);
  if( PDI_OK != ready ){
    return ready;
  }

  opened = m_bus->begin(m_cs, m_clock, 0);
  if( PDI_OK != opened ){
    return opened;
  }

  m_highcapacity = false;
  if( v2 ){
    r1 = command(58, 0);
    if( 0 != r1 ){
      finish();
      return answerError(r1);
    }
    uint8_t ocr[4] = {0, 0, 0, 0};
    m_bus->exchange(nullptr, ocr, sizeof(ocr));
    m_highcapacity = (0 != (ocr[0] & 0x40));
  }

  if( !m_highcapacity ){
    r1 = command(16, SD_BLOCK_SIZE);
    if( 0 != r1 ){
      finish();
      return answerError(r1);
    }
  }

  r1 = command(9, 0);
  if( 0 != r1 ){
    finish();
    return answerError(r1);
  }
  uint8_t csd[16];
  pdi_err_t got = receive(csd, sizeof(csd));
  finish();
  if( PDI_OK != got ){
    return got;
  }

  uint8_t structure = csd[0] >> 6;
  if( structure > 1 ){
    return PDI_ERR_NOT_SUPPORTED;
  }
  if( 1 == structure ){
    uint32_t size = ((uint32_t)(csd[7] & 0x3F) << 16) | ((uint32_t)csd[8] << 8) | csd[9];
    m_blocks = (size + 1) * 1024;
  }else{
    uint32_t size = ((uint32_t)(csd[6] & 0x03) << 10) | ((uint32_t)csd[7] << 2) | (csd[8] >> 6);
    uint8_t mult = (uint8_t)(((csd[9] & 0x03) << 1) | (csd[10] >> 7));
    uint8_t blocklen = csd[5] & 0x0F;
    m_blocks = (uint32_t)(((uint64_t)(size + 1) << (mult + 2) << blocklen) / SD_BLOCK_SIZE);
  }

  return (0 == m_blocks) ? PDI_ERR_IO : PDI_OK;
}

/**
 * Asks the card to check CRCs and send real ones with its data, carrying
 * on without data CRC checks when it does not know how.
 */
pdi_err_t MmcSpiDriver::enableCrc()
{
  m_crc = false;
  int16_t r1 = command(59, 1);
  if( r1 < 0 ){
    return answerError(r1);
  }
  if( 0 != (r1 & s_sd_illegal) ){
    SysLogW("SD: card does not check CRC, its data CRC is not checked either\n");
    return PDI_OK;
  }
  if( 0 != (r1 & ~s_sd_idle) ){
    return PDI_ERR_IO;
  }
  m_crc = true;
  return PDI_OK;
}

/**
 * Waits for the card to leave its idle state.
 */
pdi_err_t MmcSpiDriver::waitReady(bool highcapacityasked)
{
  uint32_t began = __i_dvc_ctrl.millis_now();

  while( true ){

    pdi_err_t opened = m_bus->begin(m_cs, m_clock, 0);
    if( PDI_OK != opened ){
      return opened;
    }

    int16_t r1 = command(55, 0);
    if( r1 >= 0 && 0 == (r1 & s_sd_illegal) ){
      r1 = command(41, highcapacityasked ? s_sd_high_capacity : 0);
    }
    finish();

    if( r1 < 0 ){
      return answerError(r1);
    }
    if( 0 != (r1 & s_sd_illegal) ){
      return PDI_ERR_NOT_SUPPORTED;
    }
    if( 0 == r1 ){
      return PDI_OK;
    }
    if( (__i_dvc_ctrl.millis_now() - began) >= SD_READY_TIMEOUT_MS ){
      return PDI_ERR_TIMEOUT;
    }
    __i_dvc_ctrl.yield();
  }
}

/**
 * Takes a data block the card sends, checking its CRC.
 */
pdi_err_t MmcSpiDriver::receive(uint8_t *buf, uint32_t len)
{
  uint32_t began = __i_dvc_ctrl.millis_now();
  uint8_t token = 0xFF;

  for( uint32_t polls = 1; ; polls++ ){
    m_bus->exchange(nullptr, &token, 1);
    if( 0xFF != token ){
      break;
    }
    if( (__i_dvc_ctrl.millis_now() - began) >= SD_READ_TIMEOUT_MS ){
      return PDI_ERR_TIMEOUT;
    }
    if( 0 == (polls % s_sd_polls_per_yield) ){
      __i_dvc_ctrl.yield();
    }
  }

  if( s_sd_data_token != token ){
    SysLogW("SD: card answered a read with error token %x\n", (unsigned int)token);
    return PDI_ERR_IO;
  }

  m_bus->exchange(nullptr, buf, len);
  uint8_t crc[2] = {0, 0};
  m_bus->exchange(nullptr, crc, sizeof(crc));

  if( !m_crc ){
    return PDI_OK;
  }

  uint16_t sent = BytesToUint16BE(crc);
  uint16_t computed = crc16CcittUpdate(0, buf, len);
  if( computed != sent ){
    SysLogW("SD: block CRC from card %x, computed %x\n", (unsigned int)sent, (unsigned int)computed);
    return PDI_ERR_CRC;
  }
  return PDI_OK;
}

/**
 * Waits for the card to finish storing a block.
 */
pdi_err_t MmcSpiDriver::waitStored()
{
  uint32_t began = __i_dvc_ctrl.millis_now();
  uint8_t line = 0x00;

  for( uint32_t polls = 1; ; polls++ ){
    m_bus->exchange(nullptr, &line, 1);
    if( 0xFF == line ){
      return PDI_OK;
    }
    if( (__i_dvc_ctrl.millis_now() - began) >= SD_WRITE_TIMEOUT_MS ){
      return PDI_ERR_TIMEOUT;
    }
    if( 0 == (polls % s_sd_polls_per_yield) ){
      __i_dvc_ctrl.yield();
    }
  }
}

/**
 * Reads one block.
 */
pdi_err_t MmcSpiDriver::readBlock(uint32_t block, uint8_t *buf)
{
  pdi_err_t opened = m_bus->begin(m_cs, m_clock, 0);
  if( PDI_OK != opened ){
    return opened;
  }

  int16_t r1 = command(17, address(block));
  if( 0 != r1 ){
    finish();
    return answerError(r1);
  }

  pdi_err_t got = receive(buf, SD_BLOCK_SIZE);
  finish();
  return got;
}

/**
 * Stores one block.
 */
pdi_err_t MmcSpiDriver::writeBlock(uint32_t block, const uint8_t *buf)
{
  pdi_err_t opened = m_bus->begin(m_cs, m_clock, 0);
  if( PDI_OK != opened ){
    return opened;
  }

  int16_t r1 = command(24, address(block));
  if( 0 != r1 ){
    finish();
    return answerError(r1);
  }

  uint8_t lead[2] = {0xFF, s_sd_data_token};
  m_bus->exchange(lead, nullptr, sizeof(lead));
  m_bus->exchange(buf, nullptr, SD_BLOCK_SIZE);
  uint16_t sum = crc16CcittUpdate(0, buf, SD_BLOCK_SIZE);
  uint8_t crc[2] = {(uint8_t)(sum >> 8), (uint8_t)(sum & 0xFF)};
  m_bus->exchange(crc, nullptr, sizeof(crc));

  uint8_t response = 0xFF;
  m_bus->exchange(nullptr, &response, 1);

  pdi_err_t stored = PDI_OK;
  if( 0xFF == response ){
    stored = PDI_ERR_NOT_FOUND;
  }else if( s_sd_crc_rejected == (response & 0x1F) ){
    stored = PDI_ERR_CRC;
  }else if( s_sd_accepted != (response & 0x1F) ){
    SysLogW("SD: card refused a write with response %x\n", (unsigned int)response);
    stored = PDI_ERR_IO;
  }else{
    stored = waitStored();
  }

  finish();
  return stored;
}

/**
 * Whole blocks read from the block given.
 */
int32_t MmcSpiDriver::readBlocks(uint32_t block, uint8_t *buf, uint32_t count)
{
  if( nullptr == buf ){
    return PDI_ERR_NULL_PTR;
  }
  if( nullptr == m_bus ){
    return PDI_ERR_NOT_INITIALIZED;
  }
  if( block >= m_blocks || count > m_blocks - block ){
    return PDI_ERR_NO_SPACE;
  }

  m_busy = true;
  for( uint32_t i = 0; i < count; i++ ){
    pdi_err_t got = readBlock(block + i, buf + i * SD_BLOCK_SIZE);
    if( PDI_OK != got ){
      m_busy = false;
      return got;
    }
    if( i + 1 < count ){
      __i_dvc_ctrl.yield();
    }
  }
  m_busy = false;
  return (int32_t)count;
}

/**
 * Whole blocks stored from the block given.
 */
int32_t MmcSpiDriver::writeBlocks(uint32_t block, const uint8_t *buf, uint32_t count)
{
  if( nullptr == buf ){
    return PDI_ERR_NULL_PTR;
  }
  if( nullptr == m_bus ){
    return PDI_ERR_NOT_INITIALIZED;
  }
  if( block >= m_blocks || count > m_blocks - block ){
    return PDI_ERR_NO_SPACE;
  }

  m_busy = true;
  for( uint32_t i = 0; i < count; i++ ){
    pdi_err_t stored = writeBlock(block + i, buf + i * SD_BLOCK_SIZE);
    if( PDI_OK != stored ){
      m_busy = false;
      return stored;
    }
    if( i + 1 < count ){
      __i_dvc_ctrl.yield();
    }
  }
  m_busy = false;
  return (int32_t)count;
}

#endif
