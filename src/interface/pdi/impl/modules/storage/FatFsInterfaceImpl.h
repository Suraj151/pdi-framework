/************************** FatFs Interface Impl *******************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

A FAT volume found on a disk node under /dev, mounted on a directory and
reached through the VFS until it is unmounted.

Author          : Suraj I.
Created Date    : 21st Oct 2026
******************************************************************************/

#ifndef _FATFS_INTERFACE_IMPL_H
#define _FATFS_INTERFACE_IMPL_H

#include <config/Config.h>

#ifdef ENABLE_FAT_FS

#include "../../../../../../external/FatFsWrapper.h"

class iBlockDriverInterface;

class FatFsInterfaceImpl : public FatFsWrapper {
public:
  FatFsInterfaceImpl();
  virtual ~FatFsInterfaceImpl() {}

  /**
   * Mounts the FAT volume a disk or partition node carries on an existing
   * directory; 0, or a negative error code.
   */
  static pdi_err_t mountDisk(const char *device, const char *dir);

  /**
   * Makes a FAT volume on a disk or partition node as mkfs.vfat does;
   * PDI_ERR_EXISTS for a partitioned whole disk.
   */
  static pdi_err_t formatDisk(const char *device);

  /**
   * The volume mounted from the FatFs drive number, nullptr when none is.
   */
  static FatFsInterfaceImpl *byDrive(uint8_t drive);

  /**
   * Whether a volume is mounted from the disk of that serial.
   */
  static bool holds(uint32_t serial);

  /**
   * The disk the volume was mounted from and its blocks, found again each call;
   * nullptr once that disk is gone.
   */
  iBlockDriverInterface *disk(uint32_t &first, uint32_t &span);

  /**
   * Keeps the error the disk answered with, so a failure names it.
   */
  void noteDiskError(int err) { m_disk_err = err; }

  void unmounted() override;

  pdiutil::string getPWD() const override { return m_dir; }
  bool setPWD(const char *path) override { return false; }
  pdiutil::string getLastPWD() const override { return m_dir; }

  void appendFileSeparator(char *path) override {}
  void appendFileSeparator(pdiutil::string &path) override {}
  bool updatePathNotations(const char *path, pdiutil::string &updatedpath) override { return false; }
  bool changeDirectory(const char *path) override { return false; }
  const char *getRootDirectory() const override { return m_dir.c_str(); }
  const char *getHomeDirectory() const override { return m_dir.c_str(); }
  const char *getTempDirectory() const override { return m_dir.c_str(); }
  bool setHomeDirectory(pdiutil::string &homedir) override { return false; }

  mimetype_t getFileMimeType(const pdiutil::string &path) override { return MIME_TYPE_TEXT_PLAIN; }
  pdiutil::string basename(const char *path) override;
  void applyFileSizeLimit(pdiutil::string &name, uint32_t sizelimit = FILE_NAME_MAX_SIZE) override {}

protected:
  uint32_t nowEpoch() override;
  int diskError() const override { return m_disk_err; }

private:
  /**
   * Lets go of the drive and the node, leaving the volume free for another.
   */
  void release();

  /**
   * Whether the window from the first block can hold a FAT volume: a partition, or
   * an unpartitioned disk.
   */
  static bool mountable(iBlockDriverInterface *disk, uint32_t first);

  pdiutil::string m_node;
  pdiutil::string m_dir;
  uint32_t m_serial;
  int m_disk_err;
};

#endif

#endif
