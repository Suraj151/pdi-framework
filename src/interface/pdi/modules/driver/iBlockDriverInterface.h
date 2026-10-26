/*************************** Block Driver Interface ***************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

A driver that presents a disk read and written in whole blocks, reached as a
node under /dev and listed by lsblk and under /sys/block.

Author          : Suraj I.
created Date    : 20th Oct 2026
******************************************************************************/

#ifndef _I_BLOCK_DRIVER_INTERFACE_H_
#define _I_BLOCK_DRIVER_INTERFACE_H_

#include "iDriverInterface.h"
#include <utility/BlockLayout.h>
#include <utility/SafeAlloc.h>

/**
 * iBlockDriverInterface class
 */
class iBlockDriverInterface : public iDriverInterface
{
public:

  /**
   * iBlockDriverInterface constructor.
   */
  iBlockDriverInterface() : m_serial(++serials()), m_scanned(false), m_table(BLOCK_TABLE_NONE), m_fs(BLOCK_FS_NONE), m_parts(0) {}

  /**
   * iBlockDriverInterface destructor.
   */
  virtual ~iBlockDriverInterface() {}

  /**
   * A number no other block driver made since boot carries, telling a rebound disk
   * from the one it replaced.
   */
  uint32_t serial() const { return m_serial; }

  /**
   * The disk a device's bound driver presents, nullptr when the device is not
   * bound or its driver presents none.
   */
  static iBlockDriverInterface *diskOf(const driver_device_t *dev) {
    if( nullptr == dev || DEVICE_STATE_BOUND != dev->m_state || nullptr == dev->m_instance ||
        DRIVER_CLASS_BLOCK != dev->m_instance->driverClass() ){
      return nullptr;
    }
    return static_cast<iBlockDriverInterface *>(dev->m_instance);
  }

  /**
   * Reads the filesystem and partitions from the first blocks once, again next look
   * when the first block read all zeros.
   */
  pdi_err_t scanLayout() {
    if( m_scanned ){
      return PDI_OK;
    }
    uint8_t *first = pdiutil::safe_new_array<uint8_t>(SD_BLOCK_SIZE);
    uint8_t *third = pdiutil::safe_new_array<uint8_t>(SD_BLOCK_SIZE);
    pdi_err_t read = (nullptr == first || nullptr == third) ? PDI_ERR_NO_MEM : readHead(0, blocks(), first, third);
    bool blank = true;
    if( PDI_OK == read ){
      m_fs = blockFsType(first, third);
      m_table = blockTable(first, blocks(), m_part, BLOCK_PARTITIONS_MAX, m_parts);
      for( uint16_t i = 0; i < SD_BLOCK_SIZE && blank; i++ ){
        blank = (0 == first[i]);
      }
    }
    for( uint8_t i = 0; PDI_OK == read && i < m_parts; i++ ){
      if( 0 == m_part[i].m_blocks ){
        continue;
      }
      read = readHead(m_part[i].m_start, m_part[i].m_blocks, first, third);
      m_part[i].m_fs = (PDI_OK == read) ? blockFsType(first, third) : (uint8_t)BLOCK_FS_NONE;
    }
    pdiutil::safe_delete_array(first);
    pdiutil::safe_delete_array(third);
    m_scanned = (PDI_OK == read) && !blank;
    return read;
  }

  /**
   * Drops what was read of the layout, so the next look reads it again.
   */
  void forgetLayout() { m_scanned = false; }

  /**
   * The partition table the disk carries.
   */
  uint8_t tableType() { return (PDI_OK == scanLayout()) ? m_table : (uint8_t)BLOCK_TABLE_NONE; }

  /**
   * The filesystem the whole disk carries, none when it is partitioned.
   */
  uint8_t fsType() { return (PDI_OK == scanLayout()) ? m_fs : (uint8_t)BLOCK_FS_NONE; }

  /**
   * The highest partition number the disk's table uses.
   */
  uint8_t partitionCount() { return (PDI_OK == scanLayout()) ? m_parts : 0; }

  /**
   * One partition, numbered by its slot from 1 as linux numbers them, nullptr
   * for an unused slot or one past the last.
   */
  const block_partition_t *partition(uint8_t number) {
    return (0 != number && number <= partitionCount() && 0 != m_part[number - 1].m_blocks) ? &m_part[number - 1] : nullptr;
  }

  /**
   * Blocks the disk holds.
   */
  virtual uint32_t blocks() const = 0;

  /**
   * Bytes in one block.
   */
  virtual uint16_t blockSize() const { return SD_BLOCK_SIZE; }

  /**
   * Whole blocks read from the block given: how many were read, or the code
   * that names what failed.
   */
  virtual int32_t readBlocks(uint32_t block, uint8_t *buf, uint32_t count) = 0;

  /**
   * Whole blocks stored from the block given: how many were stored, or the
   * code that names what failed.
   */
  virtual int32_t writeBlocks(uint32_t block, const uint8_t *buf, uint32_t count) = 0;

  /**
   * A block driver presents a disk.
   */
  uint8_t driverClass() const override { return DRIVER_CLASS_BLOCK; }

private:

  /**
   * The last serial handed out.
   */
  static uint32_t &serials() {
    static uint32_t last = 0;
    return last;
  }

  /**
   * Reads the first and third block of a span, the third zero when the span
   * is shorter.
   */
  pdi_err_t readHead(uint32_t start, uint32_t length, uint8_t *first, uint8_t *third) {
    int32_t got = readBlocks(start, first, 1);
    if( got < 0 ){
      return (pdi_err_t)got;
    }
    memset(third, 0, SD_BLOCK_SIZE);
    got = (length > 2) ? readBlocks(start + 2, third, 1) : 1;
    return (got < 0) ? (pdi_err_t)got : PDI_OK;
  }

  uint32_t m_serial;
  bool m_scanned;
  uint8_t m_table;
  uint8_t m_fs;
  uint8_t m_parts;
  block_partition_t m_part[BLOCK_PARTITIONS_MAX];
};

#endif
