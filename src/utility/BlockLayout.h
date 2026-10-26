/******************************** Block Layout *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

What a block device carries, read from its first blocks: the filesystem its
signature names, read the way linux blkid reads it, and the partitions a dos
partition table lists.

Author          : Suraj I.
created Date    : 21st Oct 2026
******************************************************************************/

#ifndef _BLOCK_LAYOUT_H_
#define _BLOCK_LAYOUT_H_

#include <config/Config.h>
#include <config/DriverConfig.h>

/**
 * The filesystem the first and third 512 byte blocks of a device name.
 */
uint8_t blockFsType(const uint8_t *first, const uint8_t *third);

/**
 * The partition table the first block holds and its partitions that fit the device,
 * each at its linux slot.
 */
uint8_t blockTable(const uint8_t *first, uint32_t blocks, block_partition_t *out, uint8_t max, uint8_t &count);

/**
 * The signatures blkid would name in one 512 byte block, as wipefs finds them; how
 * many were put in out.
 */
uint8_t blockSignatures(const uint8_t *block, uint32_t index, block_signature_t *out, uint8_t max);

/**
 * Writes a dos partition table into the first block, keeping the boot code ahead of
 * it.
 */
void blockTableBuild(uint8_t *first, const block_partition_t *parts, uint8_t count, uint32_t diskid);

/**
 * The identifier a dos partition table carries.
 */
uint32_t blockDiskId(const uint8_t *first);

/**
 * Reads one sfdisk script line, "start, size, type, bootable", empty fields taking
 * defaults; PDI_OK or the error.
 */
pdi_err_t blockScriptLine(const char *line, uint32_t blocks, uint32_t from, block_partition_t &out);

/**
 * The name of a disk's partition, e.g. mmcblk0p1 for partition 1 of mmcblk0.
 */
pdiutil::string blockPartitionName(const char *disk, uint8_t number);

/**
 * The name fdisk gives a partition type id, a read only string.
 */
const char *blockPartitionTypeName(uint8_t id);


#endif
