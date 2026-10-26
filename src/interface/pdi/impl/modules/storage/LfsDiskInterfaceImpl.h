/************************** LittleFS Disk Interface ***************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

A littlefs volume on a disk or partition node under /dev, made by mkfs, mounted
on a directory and reached through the VFS until it is unmounted. It keeps the
owner, mode and times of every entry the way the root filesystem does.

Author          : Suraj I.
Created Date    : 23rd Oct 2026
******************************************************************************/

#ifndef _LFS_DISK_INTERFACE_IMPL_H
#define _LFS_DISK_INTERFACE_IMPL_H

#include <config/Config.h>

#ifdef ENABLE_LFS_DISK

#include "FileSystemInterfaceImpl.h"

class iBlockDriverInterface;

/**
 * The blocks of a disk or partition node, found again on every access so a removed
 * disk answers with an error.
 */
class LfsDiskWindow : public iStorageInterface {
public:
  LfsDiskWindow(const char *node, uint32_t serial, uint32_t span) : m_node(node), m_serial(serial), m_span(span) {}

  int64_t read(uint64_t address, void *buffer, uint64_t size) override;
  int64_t write(uint64_t address, const void *buffer, uint64_t size) override;
  bool erase(uint64_t address, uint64_t size) override { return true; }
  uint64_t size() const override { return (uint64_t)m_span * SD_BLOCK_SIZE; }

  const pdiutil::string &node() const { return m_node; }
  uint32_t serial() const { return m_serial; }

private:
  /**
   * The disk and its first block for whole sectors inside the window, or the
   * code that refuses them.
   */
  pdi_err_t locate(uint64_t address, uint64_t size, iBlockDriverInterface *&disk, uint32_t &block);

  pdiutil::string m_node;
  uint32_t m_serial;
  uint32_t m_span;
};

/**
 * Holds the window so it is built before the filesystem that reads through it.
 */
struct LfsDiskWindowHolder {
  LfsDiskWindowHolder(const char *node, uint32_t serial, uint32_t span) : m_window(node, serial, span) {}

  LfsDiskWindow m_window;
};

class LfsDiskInterfaceImpl : private LfsDiskWindowHolder, public FileSystemInterfaceImpl {
public:
  LfsDiskInterfaceImpl(const char *node, uint32_t serial, uint32_t span);
  virtual ~LfsDiskInterfaceImpl() {}

  /**
   * Mounts the littlefs volume a disk or partition node carries on an existing
   * directory; 0, or a negative error code.
   */
  static pdi_err_t mountDisk(const char *device, const char *dir);

  /**
   * Makes an empty littlefs volume on a disk or partition node; PDI_ERR_EXISTS for
   * a partitioned whole disk.
   */
  static pdi_err_t formatDisk(const char *device);

  /**
   * Whether a volume is mounted from the disk of that serial.
   */
  static bool holds(uint32_t serial);

  void unmounted() override;

private:
  /**
   * The disk a node under /dev reaches, its name and its blocks; nullptr when the
   * path names no disk or partition.
   */
  static iBlockDriverInterface *window(const char *device, const char *&node, uint32_t &first, uint32_t &span);

  /**
   * The littlefs layout of a disk: sector sized reads and writes, and no wear
   * levelling of its own.
   */
  static void diskConfig(lfs_config &cfg);
};

#endif

#endif
