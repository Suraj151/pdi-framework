/******************************** Disk Tools **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 23rd Oct 2026
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_DEVFS) && defined(ENABLE_DRIVER_CORE)

#include "DiskTools.h"
#include "DevFs.h"
#include <interface/pdi.h>
#ifdef ENABLE_DISK_FS
#include <helpers/ProcHelper.h>
#endif
#ifdef ENABLE_FAT_FS
#include "FatFsInterfaceImpl.h"
#endif
#ifdef ENABLE_LFS_DISK
#include "LfsDiskInterfaceImpl.h"
#endif

namespace {

const uint8_t s_signature_blocks[] = {0, 1, 2, 4, 8};

/**
 * Reads the first block of the disk and the partitions its dos table lists;
 * whether it carries a dos table.
 */
bool readTable(iBlockDriverInterface *disk, uint8_t *first, block_partition_t *parts, uint8_t &count) {
    if (disk->readBlocks(0, first, 1) < 0) {
        count = 0;
        return false;
    }
    return BLOCK_TABLE_DOS == blockTable(first, disk->blocks(), parts, BLOCK_PARTITIONS_MAX, count);
}

#ifdef ENABLE_DISK_FS
/**
 * Whether the device is mounted on the directory already, the way mount -a
 * tells a line it has done from one it has not.
 */
bool mountedOn(const pdiutil::string &device, const pdiutil::string &dir) {
    const char *node = DevFs::nodeOf(device.c_str());
    if (nullptr == node) {
        return false;
    }
    for (uint8_t i = 0; i < __i_fs.getMountCount(); i++) {
        const vfs_mount_t *m = __i_fs.getMount(i);
        if (nullptr != m && dir == m->m_prefix && 0 == strcmp(node, m->m_name)) {
            return true;
        }
    }
    return false;
}
#endif

}

/**
 * The disk a disk or partition node under /dev reaches and the blocks it covers;
 * nullptr when the path names neither.
 */
iBlockDriverInterface *DiskTools::open(const char *device, uint32_t &first, uint32_t &span) {
    if (nullptr == device) {
        return nullptr;
    }
    const char *node = DevFs::nodeOf(device);
    return (nullptr == node) ? nullptr : __i_devfs.blockWindow(node, first, span);
}

/**
 * Whether a volume read from the disk is mounted.
 */
bool DiskTools::inUse(const iBlockDriverInterface *disk) {
    if (nullptr == disk) {
        return false;
    }
#ifdef ENABLE_FAT_FS
    if (FatFsInterfaceImpl::holds(disk->serial())) {
        return true;
    }
#endif
#ifdef ENABLE_LFS_DISK
    if (LfsDiskInterfaceImpl::holds(disk->serial())) {
        return true;
    }
#endif
    return false;
}

/**
 * The filesystem the window from the first block carries; none for a whole disk
 * that is partitioned.
 */
uint8_t DiskTools::fsType(iBlockDriverInterface *disk, uint32_t first) {
    if (nullptr == disk) {
        return BLOCK_FS_NONE;
    }
    if (0 == first) {
        return (BLOCK_TABLE_NONE == disk->tableType()) ? disk->fsType() : (uint8_t)BLOCK_FS_NONE;
    }
    for (uint8_t n = 1; n <= disk->partitionCount(); n++) {
        const block_partition_t *part = disk->partition(n);
        if (nullptr != part && part->m_start == first) {
            return part->m_fs;
        }
    }
    return BLOCK_FS_NONE;
}

#ifdef ENABLE_DISK_FS
/**
 * Mounts the filesystem a disk or partition node carries on an existing directory:
 * littlefs, or FAT for anything else.
 */
pdi_err_t DiskTools::mount(const char *device, const char *dir) {
    uint32_t first = 0, span = 0;
    iBlockDriverInterface *disk = open(device, first, span);
    if (nullptr == disk) {
        return PDI_ERR_NOT_FOUND;
    }
#ifdef ENABLE_LFS_DISK
    if (BLOCK_FS_LITTLEFS == fsType(disk, first)) {
        return LfsDiskInterfaceImpl::mountDisk(device, dir);
    }
#endif
#ifdef ENABLE_FAT_FS
    return FatFsInterfaceImpl::mountDisk(device, dir);
#else
    return PDI_ERR_NOT_SUPPORTED;
#endif
}

/**
 * Mounts each /etc/fstab line not marked noauto and not yet mounted, as mount -a
 * does; PDI_OK or the last required failure.
 */
pdi_err_t DiskTools::mountFstab(pdiutil::function<void(const pdiutil::string &, pdi_err_t)> failed) {
    pdi_err_t result = PDI_OK;
    pdiutil::string device, dir, type, options;
    pdiutil::string automatic = CHARPTR_WRAP(FSTAB_TYPE_AUTO);
    pdiutil::string noauto = CHARPTR_WRAP(FSTAB_OPTION_NOAUTO);
    pdiutil::string nofail = CHARPTR_WRAP(FSTAB_OPTION_NOFAIL);

    __i_fs.beginPrivileged();
    readProcLines(FSTAB_PATH, [&](pdiutil::string &line)->bool{
        if (!__line_field(line, 0, device) || LINE_COMMENT_CHAR == device[0]) {
            return true;
        }
        __line_field(line, 1, dir);
        __line_field(line, 2, type);
        __line_field(line, 3, options);
        __strip_trailing(dir, PATH_SEPARATOR_CHAR);
        if (__list_contains(options, FSTAB_OPTION_SEPARATOR, noauto) || (!dir.empty() && mountedOn(device, dir))) {
            return true;
        }

        pdi_err_t err = dir.empty() ? PDI_ERR_INVALID_ARG : PDI_OK;
        uint32_t first = 0, span = 0;
        iBlockDriverInterface *disk = (PDI_OK == err) ? open(device.c_str(), first, span) : nullptr;
        uint8_t fs = fsType(disk, first);
        if (PDI_OK == err && !type.empty() && type != automatic && BLOCK_FS_NONE != fs && blockFsTypeName(fs) != type) {
            err = PDI_ERR_NOT_SUPPORTED;
        }
        if (PDI_OK == err) {
            err = mount(device.c_str(), dir.c_str());
        }
        if (PDI_OK != err && !__list_contains(options, FSTAB_OPTION_SEPARATOR, nofail)) {
            result = err;
            if (nullptr != failed) {
                failed(device, err);
            }
        }
        return true;
    });
    __i_fs.endPrivileged();
    return result;
}
#endif

/**
 * Finds the signatures blkid would name in the blocks from first, telling found and
 * erasing them when set; how many.
 */
int16_t DiskTools::wipe(iBlockDriverInterface *disk, uint32_t first, uint32_t span, bool erase,
                        pdiutil::function<void(const block_signature_t &, const uint8_t *)> found) {
    if (nullptr == disk) {
        return PDI_ERR_NULL_PTR;
    }
    uint8_t *block = pdiutil::safe_new_array<uint8_t>(SD_BLOCK_SIZE);
    if (nullptr == block) {
        return PDI_ERR_NO_MEM;
    }

    int16_t total = 0;
    for (uint8_t i = 0; i < sizeof(s_signature_blocks) && total >= 0; i++) {
        uint32_t index = s_signature_blocks[i];
        if (index >= span) {
            continue;
        }
        int32_t got = disk->readBlocks(first + index, block, 1);
        if (got < 0) {
            total = (int16_t)got;
            break;
        }
        block_signature_t signatures[BLOCK_SIGNATURES_MAX];
        uint8_t count = blockSignatures(block, index, signatures, BLOCK_SIGNATURES_MAX);
        for (uint8_t s = 0; s < count; s++) {
            if (nullptr != found) {
                found(signatures[s], block + signatures[s].m_offset);
            }
            memset(block + signatures[s].m_offset, 0, signatures[s].m_size);
            total++;
        }
        if (0 == count || !erase) {
            continue;
        }
        int32_t put = disk->writeBlocks(first + index, block, 1);
        if (put < 0) {
            total = (int16_t)put;
        }
    }

    pdiutil::safe_delete_array(block);
    if (erase) {
        disk->forgetLayout();
    }
    return total;
}

/**
 * The identifier the disk's dos partition table carries.
 */
pdi_err_t DiskTools::diskId(iBlockDriverInterface *disk, uint32_t &id) {
    if (nullptr == disk) {
        return PDI_ERR_NULL_PTR;
    }
    uint8_t *first = pdiutil::safe_new_array<uint8_t>(SD_BLOCK_SIZE);
    if (nullptr == first) {
        return PDI_ERR_NO_MEM;
    }
    int32_t got = disk->readBlocks(0, first, 1);
    id = (got < 0) ? 0 : blockDiskId(first);
    pdiutil::safe_delete_array(first);
    return (got < 0) ? (pdi_err_t)got : PDI_OK;
}

/**
 * Writes the partitions as the disk's dos table, keeping an existing table's
 * identifier and boot code.
 */
pdi_err_t DiskTools::writeTable(iBlockDriverInterface *disk, const block_partition_t *parts, uint8_t count,
                                bool &created, uint32_t &diskid) {
    if (nullptr == disk) {
        return PDI_ERR_NULL_PTR;
    }
    uint8_t *first = pdiutil::safe_new_array<uint8_t>(SD_BLOCK_SIZE);
    if (nullptr == first) {
        return PDI_ERR_NO_MEM;
    }

    block_partition_t existing[BLOCK_PARTITIONS_MAX];
    uint8_t listed = 0;
    created = !readTable(disk, first, existing, listed);
    if (created) {
        memset(first, 0, SD_BLOCK_SIZE);
        do {
            diskid = __i_dvc_ctrl.random_now();
        } while (0 == diskid);
    } else {
        diskid = blockDiskId(first);
    }
    blockTableBuild(first, parts, count, diskid);

    int32_t put = disk->writeBlocks(0, first, 1);
    pdiutil::safe_delete_array(first);
    disk->forgetLayout();
    return (put < 0) ? (pdi_err_t)put : PDI_OK;
}

/**
 * Empties one slot of the disk's partition table, or all for 0; PDI_ERR_NOT_FOUND
 * when there is no table or slot.
 */
pdi_err_t DiskTools::deletePartition(iBlockDriverInterface *disk, uint8_t number) {
    if (nullptr == disk) {
        return PDI_ERR_NULL_PTR;
    }
    uint8_t *first = pdiutil::safe_new_array<uint8_t>(SD_BLOCK_SIZE);
    if (nullptr == first) {
        return PDI_ERR_NO_MEM;
    }

    block_partition_t parts[BLOCK_PARTITIONS_MAX];
    uint8_t count = 0;
    pdi_err_t result = PDI_OK;
    if (!readTable(disk, first, parts, count)) {
        result = PDI_ERR_NOT_FOUND;
    } else if (0 != number && (number > count || 0 == parts[number - 1].m_blocks)) {
        result = PDI_ERR_NOT_FOUND;
    } else {
        for (uint8_t i = 0; i < count; i++) {
            if (0 == number || i + 1 == number) {
                parts[i] = block_partition_t();
            }
        }
        blockTableBuild(first, parts, count, blockDiskId(first));
        int32_t put = disk->writeBlocks(0, first, 1);
        result = (put < 0) ? (pdi_err_t)put : PDI_OK;
    }

    pdiutil::safe_delete_array(first);
    disk->forgetLayout();
    return result;
}

#endif
