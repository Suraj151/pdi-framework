/******************************** Disk Tools **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

What wipefs, sfdisk and mkfs do to a disk: find it from its node under /dev,
tell whether a volume on it is mounted, erase the signatures it carries and
write its partition table.

Author          : Suraj I.
created Date    : 23rd Oct 2026
******************************************************************************/
#ifndef _DISK_TOOLS_H_
#define _DISK_TOOLS_H_

#include <config/Config.h>

#if defined(ENABLE_DEVFS) && defined(ENABLE_DRIVER_CORE)

#include <interface/pdi/modules/driver/iBlockDriverInterface.h>
#include "DevFs.h"

/**
 * DiskTools
 */
struct DiskTools {

  /**
   * The disk a disk or partition node under /dev reaches and the blocks it covers;
   * nullptr when the path names neither.
   */
  static iBlockDriverInterface *open(const char *device, uint32_t &first, uint32_t &span);

  /**
   * Whether a volume read from the disk is mounted.
   */
  static bool inUse(const iBlockDriverInterface *disk);

  /**
   * The filesystem the window from the first block carries; none for a whole disk
   * that is partitioned.
   */
  static uint8_t fsType(iBlockDriverInterface *disk, uint32_t first);

#ifdef ENABLE_DISK_FS
  /**
   * Mounts the filesystem a disk or partition node carries on an existing
   * directory: littlefs, or FAT for anything else.
   */
  static pdi_err_t mount(const char *device, const char *dir);

  /**
   * Mounts each /etc/fstab line not marked noauto and not yet mounted, as mount -a
   * does; PDI_OK or the last required failure.
   */
  static pdi_err_t mountFstab(pdiutil::function<void(const pdiutil::string &, pdi_err_t)> failed);
#endif

  /**
   * Finds the signatures blkid would name in the blocks from first, telling found
   * and erasing them when set; how many.
   */
  static int16_t wipe(iBlockDriverInterface *disk, uint32_t first, uint32_t span, bool erase,
                      pdiutil::function<void(const block_signature_t &, const uint8_t *)> found);

  /**
   * The identifier the disk's dos partition table carries.
   */
  static pdi_err_t diskId(iBlockDriverInterface *disk, uint32_t &id);

  /**
   * Writes the partitions as the disk's dos table, keeping an existing table's
   * identifier and boot code.
   */
  static pdi_err_t writeTable(iBlockDriverInterface *disk, const block_partition_t *parts, uint8_t count,
                              bool &created, uint32_t &diskid);

  /**
   * Empties one slot of the disk's partition table, or all for 0; PDI_ERR_NOT_FOUND
   * when there is no table or slot.
   */
  static pdi_err_t deletePartition(iBlockDriverInterface *disk, uint8_t number);
};

#endif

#endif
