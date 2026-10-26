/************************** FatFs Interface Impl *******************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

A FAT volume found on a disk node under /dev, mounted on a directory and
reached through the VFS until it is unmounted.

Author          : Suraj I.
Created Date    : 21st Oct 2026
******************************************************************************/
#include <config/Config.h>

#ifdef ENABLE_FAT_FS

#include "FatFsInterfaceImpl.h"
#include "../../../../../../external/FatFsWrapper.cpp"
#include "DevFs.h"
#include "VfsDispatcher.h"
#include <interface/pdi/modules/driver/iBlockDriverInterface.h>
#include <interface/pdi/middlewares/iNtpInterface.h>
#include <service_provider/session/SessionManager.h>

#ifdef DEVICE_PROVIDES_FATFS
#include <diskio_impl.h>
#else
#include "../../../../../../external/fatfs/diskio.h"
#endif

namespace {

class FatFsNullStorage : public iStorageInterface {
public:
    int64_t read(uint64_t, void*, uint64_t) override { return PDI_ERR_NOT_SUPPORTED; }
    int64_t write(uint64_t, const void*, uint64_t) override { return PDI_ERR_NOT_SUPPORTED; }
    bool erase(uint64_t, uint64_t) override { return false; }
    uint64_t size() const override { return 0; }
};

FatFsNullStorage s_fat_null_storage;

FatFsInterfaceImpl *s_fat_volumes[FAT_VOLUMES_MAX];

DSTATUS fatDiskStatus(BYTE pdrv) {
    uint32_t first = 0, span = 0;
    FatFsInterfaceImpl *vol = FatFsInterfaceImpl::byDrive(pdrv);
    return (nullptr != vol && nullptr != vol->disk(first, span)) ? (DSTATUS)0 : (DSTATUS)(STA_NOINIT | STA_NODISK);
}

DRESULT fatDiskRead(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    uint32_t first = 0, span = 0;
    FatFsInterfaceImpl *vol = FatFsInterfaceImpl::byDrive(pdrv);
    iBlockDriverInterface *disk = (nullptr == vol) ? nullptr : vol->disk(first, span);
    if (nullptr == disk) {
        return (nullptr == vol) ? RES_NOTRDY : RES_ERROR;
    }
    if ((uint64_t)sector + count > span) {
        return RES_PARERR;
    }
    int32_t got = disk->readBlocks(first + (uint32_t)sector, buff, count);
    if (got < 0) {
        vol->noteDiskError(got);
        return RES_ERROR;
    }
    return RES_OK;
}

DRESULT fatDiskWrite(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    uint32_t first = 0, span = 0;
    FatFsInterfaceImpl *vol = FatFsInterfaceImpl::byDrive(pdrv);
    iBlockDriverInterface *disk = (nullptr == vol) ? nullptr : vol->disk(first, span);
    if (nullptr == disk) {
        return (nullptr == vol) ? RES_NOTRDY : RES_ERROR;
    }
    if ((uint64_t)sector + count > span) {
        return RES_PARERR;
    }
    int32_t put =disk->writeBlocks(first + (uint32_t)sector, buff, count);
    if (put < 0) {
        vol->noteDiskError(put);
        return RES_ERROR;
    }
    return RES_OK;
}

DRESULT fatDiskIoctl(BYTE pdrv, BYTE cmd, void *buff) {
    uint32_t first = 0, span = 0;
    FatFsInterfaceImpl *vol = FatFsInterfaceImpl::byDrive(pdrv);
    iBlockDriverInterface *disk = (nullptr == vol) ? nullptr : vol->disk(first, span);
    if (nullptr == disk) {
        return RES_NOTRDY;
    }
    switch (cmd) {
    case CTRL_SYNC:
        return RES_OK;
    case GET_SECTOR_COUNT:
        *(LBA_t *)buff = (LBA_t)span;
        return RES_OK;
    case GET_SECTOR_SIZE:
        *(WORD *)buff = (WORD)disk->blockSize();
        return RES_OK;
    case GET_BLOCK_SIZE:
        *(DWORD *)buff = 1;
        return RES_OK;
    default:
        return RES_PARERR;
    }
}

#ifdef DEVICE_PROVIDES_FATFS
DSTATUS fatDiskInit(BYTE pdrv) {
    return fatDiskStatus(pdrv);
}

const ff_diskio_impl_t s_fat_diskio = {
    fatDiskInit, fatDiskStatus, fatDiskRead, fatDiskWrite, fatDiskIoctl
};
#endif

}

#ifndef DEVICE_PROVIDES_FATFS
extern "C" {

DSTATUS disk_initialize(BYTE pdrv) {
    return fatDiskStatus(pdrv);
}

DSTATUS disk_status(BYTE pdrv) {
    return fatDiskStatus(pdrv);
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    return fatDiskRead(pdrv, buff, sector, count);
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    return fatDiskWrite(pdrv, buff, sector, count);
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    return fatDiskIoctl(pdrv, cmd, buff);
}

DWORD get_fattime(void) {
    return __i_ntp.is_valid_ntptime() ? epochToFatTime((uint32_t)__i_ntp.get_ntp_time()) : ((DWORD)s_fat_date_unset << 16);
}

void *ff_memalloc(UINT msize) {
    return pdiutil::safe_new_array<uint8_t>(msize);
}

void ff_memfree(void *mblock) {
    uint8_t *block = (uint8_t *)mblock;
    pdiutil::safe_delete_array(block);
}

}
#endif

FatFsInterfaceImpl::FatFsInterfaceImpl() : FatFsWrapper(s_fat_null_storage), m_serial(0), m_disk_err(0) {}

/**
 * Makes a FAT volume on a disk or partition node as mkfs.vfat does; PDI_ERR_EXISTS
 * for a partitioned whole disk.
 */
pdi_err_t FatFsInterfaceImpl::formatDisk(const char *device) {
    if (nullptr == device) {
        return PDI_ERR_NULL_PTR;
    }
    const char *node = DevFs::nodeOf(device);
    if (nullptr == node) {
        return PDI_ERR_NOT_FOUND;
    }
    uint32_t first = 0, span = 0;
    iBlockDriverInterface *found = __i_devfs.blockWindow(node, first, span);
    if (nullptr == found) {
        return PDI_ERR_NOT_FOUND;
    }
    if (0 == first && BLOCK_TABLE_NONE != found->tableType()) {
        return PDI_ERR_EXISTS;
    }

    int8_t slot = -1;
    for (uint8_t i = 0; i < FAT_VOLUMES_MAX && slot < 0; i++) {
        slot = (nullptr == s_fat_volumes[i]) ? (int8_t)i : slot;
    }
    if (slot < 0) {
        return PDI_ERR_FULL;
    }

    uint8_t drive = (uint8_t)slot;
#ifdef DEVICE_PROVIDES_FATFS
    BYTE pdrv = FF_DRV_NOT_USED;
    if (ESP_OK != ff_diskio_get_drive(&pdrv) || FF_DRV_NOT_USED == pdrv) {
        return PDI_ERR_FULL;
    }
#endif

    FatFsInterfaceImpl *vol = pdiutil::safe_new<FatFsInterfaceImpl>();
    if (nullptr == vol) {
        return PDI_ERR_NO_MEM;
    }
#ifdef DEVICE_PROVIDES_FATFS
    ff_diskio_register(pdrv, &s_fat_diskio);
    drive = (uint8_t)pdrv;
#endif

    vol->m_node = node;
    vol->m_serial = found->serial();
    vol->m_disk_err = 0;
    s_fat_volumes[slot] = vol;
    pdi_err_t err = vol->formatVolume(drive, span >= FAT_FORMAT_FAT32_SECTORS);
    vol->release();
    s_fat_volumes[slot] = nullptr;
    pdiutil::safe_delete(vol);
    found->forgetLayout();
    return err;
}

/**
 * Whether a volume is mounted from the disk of that serial.
 */
bool FatFsInterfaceImpl::holds(uint32_t serial) {
    for (uint8_t i = 0; i < FAT_VOLUMES_MAX; i++) {
        if (nullptr != s_fat_volumes[i] && s_fat_volumes[i]->m_serial == serial) {
            return true;
        }
    }
    return false;
}

/**
 * Mounts the FAT volume a disk or partition node carries on an existing directory;
 * 0, or a negative error code.
 */
pdi_err_t FatFsInterfaceImpl::mountDisk(const char *device, const char *dir) {
    if (nullptr == device || nullptr == dir) {
        return PDI_ERR_NULL_PTR;
    }
    const char *node = DevFs::nodeOf(device);
    if (nullptr == node) {
        return PDI_ERR_NOT_FOUND;
    }
    uint32_t first = 0, span = 0;
    iBlockDriverInterface *found = __i_devfs.blockWindow(node, first, span);
    if (nullptr == found) {
        return PDI_ERR_NOT_FOUND;
    }
    if (!mountable(found, first)) {
        return PDI_ERR_NOT_SUPPORTED;
    }

    int8_t slot = -1;
    for (uint8_t i = 0; i < FAT_VOLUMES_MAX; i++) {
        if (nullptr != s_fat_volumes[i] && s_fat_volumes[i]->m_node == node) {
            return PDI_ERR_BUSY;
        }
        slot = (slot < 0 && nullptr == s_fat_volumes[i]) ? (int8_t)i : slot;
    }
    if (!__i_fs.isDirExist(dir)) {
        return STORAGE_ERROR_NOT_A_DIRECTORY;
    }
    if (slot < 0) {
        return PDI_ERR_FULL;
    }

    uint8_t drive = (uint8_t)slot;
#ifdef DEVICE_PROVIDES_FATFS
    BYTE pdrv = FF_DRV_NOT_USED;
    if (ESP_OK != ff_diskio_get_drive(&pdrv) || FF_DRV_NOT_USED == pdrv) {
        return PDI_ERR_FULL;
    }
#endif

    FatFsInterfaceImpl *vol = pdiutil::safe_new<FatFsInterfaceImpl>();
    if (nullptr == vol) {
        return PDI_ERR_NO_MEM;
    }
#ifdef DEVICE_PROVIDES_FATFS
    ff_diskio_register(pdrv, &s_fat_diskio);
    drive = (uint8_t)pdrv;
#endif

    vol->m_node = node;
    vol->m_dir = dir;
    vol->m_serial = found->serial();
    vol->m_disk_err = 0;
    s_fat_volumes[slot] = vol;
    pdi_err_t err = vol->mountVolume(drive);
    if (PDI_OK == err) {
        err = (pdi_err_t)__i_fs.mount(dir, vol, node, VFS_TYPE_VFAT, true);
        if (PDI_OK != err) {
            vol->unmountVolume();
        }
    }
    if (PDI_OK != err) {
        vol->release();
        s_fat_volumes[slot] = nullptr;
        pdiutil::safe_delete(vol);
    }
    return err;
}

/**
 * Whether the window from the first block can hold a FAT volume: a partition, or an
 * unpartitioned disk.
 */
bool FatFsInterfaceImpl::mountable(iBlockDriverInterface *disk, uint32_t first) {
    uint8_t fs = BLOCK_FS_NONE;
    if (0 == first) {
        if (BLOCK_TABLE_NONE != disk->tableType()) {
            return false;
        }
        fs = disk->fsType();
    }
    for (uint8_t n = 1; 0 != first && n <= disk->partitionCount(); n++) {
        const block_partition_t *part = disk->partition(n);
        fs = (nullptr != part && part->m_start == first) ? part->m_fs : fs;
    }
    return BLOCK_FS_NONE == fs || BLOCK_FS_VFAT == fs;
}

/**
 * The volume mounted from the FatFs drive number, nullptr when none is.
 */
FatFsInterfaceImpl *FatFsInterfaceImpl::byDrive(uint8_t drive) {
    for (uint8_t i = 0; i < FAT_VOLUMES_MAX; i++) {
        if (nullptr != s_fat_volumes[i] && !s_fat_volumes[i]->m_node.empty() && s_fat_volumes[i]->drive() == drive) {
            return s_fat_volumes[i];
        }
    }
    return nullptr;
}

/**
 * The disk the volume was mounted from and its blocks, found again each call;
 * nullptr once that disk is gone.
 */
iBlockDriverInterface *FatFsInterfaceImpl::disk(uint32_t &first, uint32_t &span) {
    iBlockDriverInterface *found = m_node.empty() ? nullptr : __i_devfs.blockWindow(m_node.c_str(), first, span);
    if (nullptr == found || found->serial() != m_serial) {
        m_disk_err = PDI_ERR_NOT_FOUND;
        return nullptr;
    }
    return found;
}

void FatFsInterfaceImpl::unmounted() {
    unmountVolume();
    release();
    for (uint8_t i = 0; i < FAT_VOLUMES_MAX; i++) {
        if (this == s_fat_volumes[i]) {
            FatFsInterfaceImpl *vol = s_fat_volumes[i];
            s_fat_volumes[i] = nullptr;
            pdiutil::safe_delete(vol);
            return;
        }
    }
}

/**
 * Lets go of the drive and the node, leaving the volume free for another.
 */
void FatFsInterfaceImpl::release() {
#ifdef DEVICE_PROVIDES_FATFS
    if (!m_node.empty()) {
        ff_diskio_unregister(drive());
    }
#endif
    m_node.clear();
    m_dir.clear();
    m_disk_err = 0;
}

pdiutil::string FatFsInterfaceImpl::basename(const char *path) {
    if (nullptr == path) return pdiutil::string();
    const char *last = path;
    for (const char *p = path; *p; ++p) {
        if (*p == PATH_SEPARATOR_CHAR) last = p + 1;
    }
    return pdiutil::string(last);
}

uint32_t FatFsInterfaceImpl::nowEpoch() {
    if (!__i_ntp.is_valid_ntptime()) return 0;
    return (uint32_t)__i_ntp.get_ntp_time();
}

#endif
