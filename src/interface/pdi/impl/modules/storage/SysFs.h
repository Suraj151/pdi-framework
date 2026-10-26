/********************************** SysFS **************************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

SysFS is a synthetic filesystem mounted at /sys. It exposes the device surfaces
as a tree of small nodes, and unlike the rest of the synthetic filesystems some
of its leaves are writable.

Author          : Suraj I.
Created Date    : 23rd July 2026
******************************************************************************/

#ifndef _SYS_FS_H
#define _SYS_FS_H

#include <config/Config.h>

#ifdef ENABLE_SYSFS

#include "SynthFs.h"

class iNvmemDriverInterface;

class SysFs : public SynthFs {
public:
  SysFs();
  virtual ~SysFs() {}

  int writeFile(const char *path, const char *content, uint32_t size,
                bool append = false) override;

#ifdef ENABLE_DRIVER_CORE
  int readFile(const char *path, uint64_t size,
               pdiutil::function<bool(char *, uint32_t)> readbackfn,
               uint64_t offset = 0, const char *readUntilMatchStr = nullptr,
               bool *didmatchfound = nullptr) override;
  int editFile(const char *path, uint64_t offset, const char *content,
               uint32_t size) override;
#endif

protected:
  synth_node_t resolve(const char *path) override;
  pdiutil::string render(const char *path) override;
  int listChildren(const char *path,
                   pdiutil::vector<file_info_t> &items) override;
  uint16_t permsFor(const char *path, synth_node_t kind) override;
#ifdef ENABLE_DRIVER_CORE
  int64_t sizeOf(const char *path) override;
#endif

private:
  enum NodeKind : uint8_t {
    SYS_INVALID = 0,
    SYS_ROOT,
    SYS_CLASS,
    SYS_GPIODIR,
    SYS_PIN,
    SYS_VALUE,
    SYS_MODE,
    SYS_NETDIR,
    SYS_NETIF,
    SYS_NETATTR,
    SYS_BUS,
    SYS_BUSDIR,
    SYS_BUSDEVS,
    SYS_BUSDEV,
    SYS_BUSDEVATTR,
    SYS_BUSDRVS,
    SYS_BUSDRV,
    SYS_BUSRESCAN,
    SYS_BUSDEVNVMEM,
    SYS_BLOCK,
    SYS_BLOCKDEV,
    SYS_BLOCKSIZE,
    SYS_BLOCKPART,
    SYS_BLOCKPARTATTR
  };

  /**
   * Which node the path names, plus the pin or interface index behind it and
   * which of that node's leaves it is.
   */
  NodeKind classify(const char *path, int16_t &index_out,
                    uint8_t &leaf_out) const;

  /**
   * Whether the pin exists on this board and is safe to drive.
   */
  bool isValidPin(uint8_t pin) const;

  /**
   * What one leaf of a registered network interface currently reads.
   */
  pdiutil::string renderNetAttr(uint8_t index, uint8_t leaf);

#ifdef ENABLE_DRIVER_CORE
  /**
   * Which node under /sys/bus the rest of the path names: a bus, its devices and
   * drivers, a device leaf, or rescan.
   */
  NodeKind classifyBus(const char *p, int16_t &index_out, uint8_t &leaf_out) const;

  /**
   * Which node under /sys/block the rest of the path names.
   */
  NodeKind classifyBlock(const char *p, int16_t &index_out, uint8_t &leaf_out) const;

  /**
   * What a disk or partition leaf under /sys/block currently reads.
   */
  pdiutil::string renderBlockAttr(uint16_t device, uint8_t leaf);

  /**
   * What one leaf of a known device currently reads.
   */
  pdiutil::string renderBusAttr(uint16_t device, uint8_t leaf);

  /**
   * The memory behind a device's eeprom file, nullptr when the path names none.
   */
  iNvmemDriverInterface *nvmemAt(const char *path) const;

  /**
   * Delivers device memory from the offset a transfer at a time, yielding between
   * and finding the device again each time.
   */
  int readNvmem(const char *path, uint64_t size,
                pdiutil::function<bool(char *, uint32_t)> readbackfn,
                uint64_t offset, const char *readUntilMatchStr, bool *didmatchfound);
#endif
};

extern SysFs __i_sysfs;

#endif // ENABLE_SYSFS

#endif
