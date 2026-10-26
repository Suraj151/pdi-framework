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

#include <config/Config.h>

#ifdef ENABLE_LFS_DISK

#include "LfsDiskInterfaceImpl.h"
#include "DevFs.h"
#include "DiskTools.h"
#include <interface/pdi.h>

namespace {

LfsDiskInterfaceImpl *s_lfs_volumes[LFS_DISK_VOLUMES_MAX];

}

int64_t LfsDiskWindow::read(uint64_t address, void *buffer, uint64_t size) {
    iBlockDriverInterface *disk = nullptr;
    uint32_t block = 0;
    pdi_err_t err = locate(address, size, disk, block);
    if (PDI_OK != err) {
        return err;
    }
    int32_t got = disk->readBlocks(block, (uint8_t *)buffer, (uint32_t)(size / SD_BLOCK_SIZE));
    return (got < 0) ? (int64_t)got : (int64_t)size;
}

int64_t LfsDiskWindow::write(uint64_t address, const void *buffer, uint64_t size) {
    iBlockDriverInterface *disk = nullptr;
    uint32_t block = 0;
    pdi_err_t err = locate(address, size, disk, block);
    if (PDI_OK != err) {
        return err;
    }
    int32_t put = disk->writeBlocks(block, (const uint8_t *)buffer, (uint32_t)(size / SD_BLOCK_SIZE));
    return (put < 0) ? (int64_t)put : (int64_t)size;
}

/**
 * The disk and its first block for whole sectors inside the window, or the
 * code that refuses them.
 */
pdi_err_t LfsDiskWindow::locate(uint64_t address, uint64_t size, iBlockDriverInterface *&disk, uint32_t &block) {
    uint32_t first = 0, span = 0;
    disk = __i_devfs.blockWindow(m_node.c_str(), first, span);
    if (nullptr == disk || disk->serial() != m_serial) {
        return PDI_ERR_NOT_FOUND;
    }
    if (0 != address % SD_BLOCK_SIZE || 0 != size % SD_BLOCK_SIZE) {
        return PDI_ERR_INVALID_ARG;
    }
    if (address + size > (uint64_t)span * SD_BLOCK_SIZE) {
        return PDI_ERR_RANGE;
    }
    block = first + (uint32_t)(address / SD_BLOCK_SIZE);
    return PDI_OK;
}

LfsDiskInterfaceImpl::LfsDiskInterfaceImpl(const char *node, uint32_t serial, uint32_t span)
    : LfsDiskWindowHolder(node, serial, span), FileSystemInterfaceImpl(m_window, false) {
}

/**
 * Mounts the littlefs volume a disk or partition node carries on an existing
 * directory; 0, or a negative error code.
 */
pdi_err_t LfsDiskInterfaceImpl::mountDisk(const char *device, const char *dir) {
    if (nullptr == dir) {
        return PDI_ERR_NULL_PTR;
    }
    const char *node = nullptr;
    uint32_t first = 0, span = 0;
    iBlockDriverInterface *found = window(device, node, first, span);
    if (nullptr == found) {
        return PDI_ERR_NOT_FOUND;
    }
    if (BLOCK_FS_LITTLEFS != DiskTools::fsType(found, first)) {
        return PDI_ERR_NOT_SUPPORTED;
    }

    int8_t slot = -1;
    for (uint8_t i = 0; i < LFS_DISK_VOLUMES_MAX; i++) {
        if (nullptr != s_lfs_volumes[i] && s_lfs_volumes[i]->m_window.node() == node) {
            return PDI_ERR_BUSY;
        }
        slot = (slot < 0 && nullptr == s_lfs_volumes[i]) ? (int8_t)i : slot;
    }
    if (!__i_fs.isDirExist(dir)) {
        return STORAGE_ERROR_NOT_A_DIRECTORY;
    }
    if (slot < 0) {
        return PDI_ERR_FULL;
    }

    LfsDiskInterfaceImpl *vol = pdiutil::safe_new<LfsDiskInterfaceImpl>(node, found->serial(), span);
    if (nullptr == vol) {
        return PDI_ERR_NO_MEM;
    }
    lfs_config cfg;
    diskConfig(cfg);
    pdi_err_t err = (pdi_err_t)vol->initLFSConfig(&cfg, false);
    if (PDI_OK == err) {
        err = (pdi_err_t)__i_fs.mount(dir, vol, node, VFS_TYPE_LITTLEFS, true);
    }
    if (PDI_OK != err) {
        pdiutil::safe_delete(vol);
        return err;
    }
    s_lfs_volumes[slot] = vol;
    return PDI_OK;
}

/**
 * Makes an empty littlefs volume on a disk or partition node; PDI_ERR_EXISTS for a
 * partitioned whole disk.
 */
pdi_err_t LfsDiskInterfaceImpl::formatDisk(const char *device) {
    const char *node = nullptr;
    uint32_t first = 0, span = 0;
    iBlockDriverInterface *found = window(device, node, first, span);
    if (nullptr == found) {
        return PDI_ERR_NOT_FOUND;
    }
    if (0 == first && BLOCK_TABLE_NONE != found->tableType()) {
        return PDI_ERR_EXISTS;
    }
    if (holds(found->serial())) {
        return PDI_ERR_BUSY;
    }

    LfsDiskInterfaceImpl *vol = pdiutil::safe_new<LfsDiskInterfaceImpl>(node, found->serial(), span);
    if (nullptr == vol) {
        return PDI_ERR_NO_MEM;
    }
    lfs_config cfg;
    diskConfig(cfg);
    pdi_err_t err = (pdi_err_t)vol->formatLFS(&cfg);
    pdiutil::safe_delete(vol);
    found->forgetLayout();
    return err;
}

/**
 * Whether a volume is mounted from the disk of that serial.
 */
bool LfsDiskInterfaceImpl::holds(uint32_t serial) {
    for (uint8_t i = 0; i < LFS_DISK_VOLUMES_MAX; i++) {
        if (nullptr != s_lfs_volumes[i] && s_lfs_volumes[i]->m_window.serial() == serial) {
            return true;
        }
    }
    return false;
}

void LfsDiskInterfaceImpl::unmounted() {
    for (uint8_t i = 0; i < LFS_DISK_VOLUMES_MAX; i++) {
        if (this == s_lfs_volumes[i]) {
            LfsDiskInterfaceImpl *vol = s_lfs_volumes[i];
            s_lfs_volumes[i] = nullptr;
            pdiutil::safe_delete(vol);
            return;
        }
    }
}

/**
 * The disk a node under /dev reaches, its name and its blocks; nullptr when the
 * path names no disk or partition.
 */
iBlockDriverInterface *LfsDiskInterfaceImpl::window(const char *device, const char *&node, uint32_t &first, uint32_t &span) {
    iBlockDriverInterface *found = DiskTools::open(device, first, span);
    node = (nullptr == found) ? nullptr : DevFs::nodeOf(device);
    return found;
}

/**
 * The littlefs layout of a disk: sector sized reads and writes, and no wear
 * levelling of its own.
 */
void LfsDiskInterfaceImpl::diskConfig(lfs_config &cfg) {
    memset(&cfg, 0, sizeof(cfg));
    cfg.read_size = LFS_DISK_IO_SIZE;
    cfg.prog_size = LFS_DISK_IO_SIZE;
    cfg.block_size = LFS_DISK_BLOCK_SIZE;
    cfg.cache_size = LFS_DISK_IO_SIZE;
    cfg.lookahead_size = LFS_DISK_LOOKAHEAD_SIZE;
    cfg.block_cycles = -1;
}

#endif
