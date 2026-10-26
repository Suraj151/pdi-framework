/********************************** DevFS **************************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 23rd July 2026
******************************************************************************/

#include <config/Config.h>

#ifdef ENABLE_DEVFS

#include "DevFs.h"
#include <interface/pdi.h>

#ifdef ENABLE_DRIVER_CORE
#include <service_provider/transport/SerialServiceProvider.h>
#include <interface/pdi/modules/driver/iBlockDriverInterface.h>
#endif

namespace {

#ifdef ENABLE_DRIVER_CORE

/**
 * The disk at the bus and target, found afresh, nullptr when it is gone.
 */
iBlockDriverInterface* diskAt(serial_iface_t bus, uint32_t target) {
    DeviceManager* mgr = __serial_service.devices();
    return (nullptr == mgr) ? nullptr : iBlockDriverInterface::diskOf(mgr->find(bus, target));
}
#endif

class DevFsNullStorage : public iStorageInterface {
public:
    int64_t read(uint64_t, void*, uint64_t) override { return PDI_ERR_NOT_SUPPORTED; }
    int64_t write(uint64_t, const void*, uint64_t) override { return PDI_ERR_NOT_SUPPORTED; }
    bool erase(uint64_t, uint64_t) override { return false; }
    uint64_t size() const override { return 0; }
};

DevFsNullStorage s_dev_null_storage;

// Plain string literals at namespace scope (RODT_ATTR is only legal inside a
// function body on esp8266). Order is irrelevant — listing only.
const char* const s_dev_files[] = {
    "null",
    "zero",
    "random",
    "urandom"
};

const uint8_t s_dev_file_count = sizeof(s_dev_files) / sizeof(s_dev_files[0]);

}

DevFs __i_devfs;

DevFs::DevFs() : iFileSystemInterface(s_dev_null_storage) {}

/**
 * The node a path under /dev names, e.g. mmcblk0p1 for /dev/mmcblk0p1;
 * nullptr for a path anywhere else.
 */
const char *DevFs::nodeOf(const char *path) {
    uint32_t prefixlen = (uint32_t)strlen(DEV_MOUNT_PREFIX);
    if (nullptr == path || 0 != strncmp(path, DEV_MOUNT_PREFIX, prefixlen) || PATH_SEPARATOR_CHAR != path[prefixlen]) {
        return nullptr;
    }
    return path + prefixlen + 1;
}

/**
 * The path under /dev of a node, e.g. /dev/mmcblk0p1 for mmcblk0p1.
 */
pdiutil::string DevFs::pathOf(const char *node) {
    pdiutil::string path = CHARPTR_WRAP(DEV_MOUNT_PREFIX);
    path += PATH_SEPARATOR_CHAR;
    if (nullptr != node) {
        path += node;
    }
    return path;
}

const char* DevFs::normalizePath(const char* path) const {
    if (!path) return "";
    while (*path == PATH_SEPARATOR_CHAR) path++;
    return path;
}

DevFs::NodeKind DevFs::classify(const char* path) const {
    const char* p = normalizePath(path);
    if (p[0] == '\0') return DEV_ROOT;
    if (strcmp(p, "null") == 0) return DEV_NULL;
    if (strcmp(p, "zero") == 0) return DEV_ZERO;
    if (strcmp(p, "random") == 0) return DEV_RANDOM;
    if (strcmp(p, "urandom") == 0) return DEV_URANDOM;
#ifdef ENABLE_DRIVER_CORE
    bool isdir = false;
    if (driverNode(p, isdir) >= 0) return isdir ? DEV_DRIVER_DIR : DEV_DRIVER_NODE;
    uint8_t number = 0;
    if (partitionNode(p, number) >= 0) return DEV_PARTITION;
#endif
    return DEV_INVALID;
}

#ifdef ENABLE_DRIVER_CORE
/**
 * The bound device whose node the path names, or the first one under the
 * directory it names; -1 when it names neither.
 */
int16_t DevFs::driverNode(const char* p, bool& isdir) const {
    DeviceManager* mgr = __serial_service.devices();
    if (nullptr == mgr || '\0' == p[0]) return -1;

    uint32_t plen = (uint32_t)strlen(p);
    pdiutil::string node;
    for (uint16_t i = 0; i < mgr->count(); ++i) {
        const driver_device_t* dev = mgr->at(i);
        if (nullptr == dev->m_instance || !dev->m_instance->nodeName(node)) continue;
        if (node == p) {
            isdir = false;
            return (int16_t)i;
        }
        if (node.size() > plen && 0 == strncmp(node.c_str(), p, plen) &&
            PATH_SEPARATOR_CHAR == node[plen]) {
            isdir = true;
            return (int16_t)i;
        }
    }
    return -1;
}

/**
 * Adds the next path segment of each bound driver's node under a driver directory
 * or the root, a directory when more follows.
 */
void DevFs::listDriverNodes(const char* p, pdiutil::vector<file_info_t>& items) const {
    DeviceManager* mgr = __serial_service.devices();
    if (nullptr == mgr) return;

    uint32_t plen = (uint32_t)strlen(p);
    pdiutil::string node;
    for (uint16_t i = 0; i < mgr->count(); ++i) {
        const driver_device_t* dev = mgr->at(i);
        if (nullptr == dev->m_instance || !dev->m_instance->nodeName(node)) continue;

        const char* rest = node.c_str();
        if (plen > 0) {
            if (node.size() <= plen || 0 != strncmp(rest, p, plen) || PATH_SEPARATOR_CHAR != rest[plen]) continue;
            rest += plen + 1;
        }

        const char* end = strchr(rest, PATH_SEPARATOR_CHAR);
        uint32_t namelen = (nullptr == end) ? (uint32_t)strlen(rest) : (uint32_t)(end - rest);
        bool seen = false;
        for (uint16_t k = 0; k < items.size(); ++k) {
            if (nullptr != items[k].m_name && strlen(items[k].m_name) == namelen &&
                0 == strncmp(items[k].m_name, rest, namelen)) {
                seen = true;
                break;
            }
        }
        if (seen) continue;

        file_info_t info;
        memset(&info, 0, sizeof(info));
        info.m_type = (nullptr == end) ? FILE_TYPE_REG : FILE_TYPE_DIR;
        info.m_perms = (nullptr == end) ? 0444 : 0555;
        iBlockDriverInterface* disk = (nullptr == end) ? iBlockDriverInterface::diskOf(dev) : nullptr;
        if (nullptr != disk) {
            info.m_perms = 0600;
            info.m_size = (int64_t)disk->blocks() * disk->blockSize();
        }
        info.m_name = pdiutil::safe_new_array<char>(namelen + 1);
        if (nullptr == info.m_name) continue;
        memcpy(info.m_name, rest, namelen);
        info.m_name[namelen] = '\0';
        items.push_back(info);

        for (uint8_t n = 1; nullptr != disk && n <= disk->partitionCount(); ++n) {
            if (nullptr == disk->partition(n)) continue;
            pdiutil::string name = blockPartitionName(pdiutil::string(rest, namelen).c_str(), n);
            file_info_t part;
            memset(&part, 0, sizeof(part));
            part.m_type = FILE_TYPE_REG;
            part.m_perms = 0600;
            part.m_size = (int64_t)disk->partition(n)->m_blocks * SD_BLOCK_SIZE;
            part.m_name = pdiutil::safe_new_array<char>(name.size() + 1);
            if (nullptr == part.m_name) continue;
            memcpy(part.m_name, name.c_str(), name.size() + 1);
            items.push_back(part);
        }
    }
}

/**
 * Delivers a bounded run of bytes read from a driver's node, capped so a reader
 * always reaches an end.
 */
int DevFs::driverRead(int16_t device, uint64_t size,
                      pdiutil::function<bool(char*, uint32_t)> readbackfn,
                      uint64_t offset) {
    DeviceManager* mgr = __serial_service.devices();
    const driver_device_t* dev = (nullptr == mgr) ? nullptr : mgr->at((uint16_t)device);
    if (nullptr == dev || nullptr == dev->m_instance) return PDI_ERR_NOT_FOUND;

    uint32_t cap = DEVFS_STREAM_READ_MAX;
    if (offset >= cap) return 0;

    uint8_t stream[DEVFS_STREAM_READ_MAX];
    uint32_t total = cap - (uint32_t)offset;
    int32_t got = dev->m_instance->readNode(stream, total);
    if (got <= 0) return (got < 0) ? (int)got : 0;

    uint32_t chunk = (size > 0 && size < (uint64_t)got) ? (uint32_t)size : (uint32_t)got;
    uint32_t done = 0;
    while (done < (uint32_t)got) {
        uint32_t n = (uint32_t)got - done;
        if (n > chunk) n = chunk;
        if (!readbackfn((char*)stream + done, n)) break;
        done += n;
    }
    return (int)done;
}

/**
 * Whether the node names a disk or one of its partitions.
 */
bool DevFs::isDisk(const char* path) const {
    int16_t device = -1;
    uint32_t first = 0, span = 0;
    return diskWindow(path, device, first, span);
}

/**
 * The disk a partition node names and the partition's number, -1 when the
 * path names none.
 */
int16_t DevFs::partitionNode(const char* p, uint8_t& number) const {
    DeviceManager* mgr = __serial_service.devices();
    if (nullptr == mgr || '\0' == p[0]) return -1;

    pdiutil::string node;
    for (uint16_t i = 0; i < mgr->count(); ++i) {
        iBlockDriverInterface* disk = iBlockDriverInterface::diskOf(mgr->at(i));
        if (nullptr == disk || !disk->nodeName(node)) continue;
        uint32_t len = (uint32_t)node.size();
        if (0 != strncmp(p, node.c_str(), len) || 'p' != p[len] || !__is_digit(p[len + 1])) continue;
        uint32_t value = StringToUint32(p + len + 1, (uint8_t)strlen(p + len + 1));
        bool digits = true;
        for (const char* c = p + len + 1; '\0' != *c; ++c) digits = digits && __is_digit(*c);
        if (!digits || value > BLOCK_PARTITIONS_MAX || nullptr == disk->partition((uint8_t)value)) continue;
        number = (uint8_t)value;
        return (int16_t)i;
    }
    return -1;
}

/**
 * The disk a disk or partition node reaches and the blocks it covers.
 */
bool DevFs::diskWindow(const char* path, int16_t& device, uint32_t& first, uint32_t& span) const {
    const char* p = normalizePath(path);
    DeviceManager* mgr = __serial_service.devices();
    if (nullptr == mgr) return false;

    bool isdir = false;
    device = driverNode(p, isdir);
    iBlockDriverInterface* disk = (device < 0 || isdir) ? nullptr : iBlockDriverInterface::diskOf(mgr->at((uint16_t)device));
    if (nullptr != disk) {
        first = 0;
        span = disk->blocks();
        return true;
    }

    uint8_t number = 0;
    device = partitionNode(p, number);
    disk = (device < 0) ? nullptr : iBlockDriverInterface::diskOf(mgr->at((uint16_t)device));
    const block_partition_t* part = (nullptr == disk) ? nullptr : disk->partition(number);
    if (nullptr == part) return false;
    first = part->m_start;
    span = part->m_blocks;
    return true;
}

/**
 * The disk a disk or partition node reaches and the blocks it covers, nullptr
 * when the node names neither.
 */
iBlockDriverInterface* DevFs::blockWindow(const char* path, uint32_t& first, uint32_t& span) const {
    int16_t device = -1;
    DeviceManager* mgr = __serial_service.devices();
    if (nullptr == mgr || !diskWindow(path, device, first, span)) return nullptr;
    return iBlockDriverInterface::diskOf(mgr->at((uint16_t)device));
}

/**
 * The bytes a disk or partition node holds, 0 for any other node.
 */
uint64_t DevFs::diskSize(const char* path) const {
    int16_t device = -1;
    uint32_t first = 0, span = 0;
    return diskWindow(path, device, first, span) ? (uint64_t)span * SD_BLOCK_SIZE : 0;
}

/**
 * Delivers a disk window from the offset a block at a time.
 */
int DevFs::diskRead(int16_t device, uint32_t first, uint32_t span, uint64_t size,
                    pdiutil::function<bool(char*, uint32_t)> readbackfn,
                    uint64_t offset, const char* readUntilMatchStr, bool* didmatchfound) {
    DeviceManager* mgr = __serial_service.devices();
    const driver_device_t* dev = (nullptr == mgr) ? nullptr : mgr->at((uint16_t)device);
    iBlockDriverInterface* disk = iBlockDriverInterface::diskOf(dev);
    if (nullptr == disk) return PDI_ERR_NOT_FOUND;

    serial_iface_t bus = dev->m_bus;
    uint32_t target = dev->m_target;
    uint64_t total = (uint64_t)span * SD_BLOCK_SIZE;
    if (offset >= total) return 0;

    bool matching = (nullptr != readUntilMatchStr && '\0' != readUntilMatchStr[0]);
    uint32_t matchlen = matching ? (uint32_t)strlen(readUntilMatchStr) : 0;
    uint32_t piece = (size > 0 && size < SD_BLOCK_SIZE) ? (uint32_t)size : SD_BLOCK_SIZE;
    uint8_t* block = pdiutil::safe_new_array<uint8_t>(SD_BLOCK_SIZE);
    if (nullptr == block) return PDI_ERR_NO_MEM;
    uint64_t at = offset;
    uint32_t done = 0;
    int result = 0;

    while (at < total) {
        disk = diskAt(bus, target);
        if (nullptr == disk) {
            result = PDI_ERR_NOT_FOUND;
            break;
        }

        int32_t got = disk->readBlocks(first + (uint32_t)(at / SD_BLOCK_SIZE), block, 1);
        if (got < 0) {
            result = (int)got;
            break;
        }

        uint32_t skip = (uint32_t)(at % SD_BLOCK_SIZE);
        uint32_t give = SD_BLOCK_SIZE - skip;
        bool matched = false;
        if (matching) {
            int32_t found = __strstr((const char*)block + skip, give, readUntilMatchStr, matchlen, 0);
            if (found >= 0) {
                give = (uint32_t)found;
                matched = true;
            }
        }

        bool stopped = false;
        for (uint32_t sent = 0; sent < give && !stopped;) {
            uint32_t n = (give - sent < piece) ? give - sent : piece;
            stopped = !readbackfn((char*)block + skip + sent, n);
            sent += n;
            done += n;
        }
        at += SD_BLOCK_SIZE - skip;

        if (stopped) break;
        if (matched) {
            if (nullptr != didmatchfound) *didmatchfound = true;
            break;
        }

        __i_dvc_ctrl.yield();
    }

    pdiutil::safe_delete_array(block);
    return (result < 0) ? result : (int)done;
}

/**
 * Stores bytes in a disk window from the offset, a block at a time.
 */
int DevFs::diskWrite(int16_t device, uint32_t first, uint32_t span, uint64_t offset, const char* content, uint32_t size) {
    DeviceManager* mgr = __serial_service.devices();
    const driver_device_t* dev = (nullptr == mgr) ? nullptr : mgr->at((uint16_t)device);
    iBlockDriverInterface* disk = iBlockDriverInterface::diskOf(dev);
    if (nullptr == disk) return PDI_ERR_NOT_FOUND;
    if (nullptr == content) return PDI_ERR_NULL_PTR;

    serial_iface_t bus = dev->m_bus;
    uint32_t target = dev->m_target;
    uint64_t total = (uint64_t)span * SD_BLOCK_SIZE;
    if (offset >= total) return PDI_ERR_NO_SPACE;
    if ((uint64_t)size > total - offset) size = (uint32_t)(total - offset);

    uint8_t* block = pdiutil::safe_new_array<uint8_t>(SD_BLOCK_SIZE);
    if (nullptr == block) return PDI_ERR_NO_MEM;
    uint32_t done = 0;
    int result = 0;

    while (done < size) {
        disk = diskAt(bus, target);
        if (nullptr == disk) {
            result = PDI_ERR_NOT_FOUND;
            break;
        }

        uint64_t at = offset + done;
        uint32_t number = (uint32_t)(at / SD_BLOCK_SIZE);
        uint32_t skip = (uint32_t)(at % SD_BLOCK_SIZE);
        uint32_t n = SD_BLOCK_SIZE - skip;
        if (n > size - done) n = size - done;

        if (n < SD_BLOCK_SIZE) {
            int32_t got = disk->readBlocks(first + number, block, 1);
            if (got < 0) {
                result = (int)got;
                break;
            }
        }
        memcpy(block + skip, content + done, n);

        int32_t stored = disk->writeBlocks(first + number, block, 1);
        disk->forgetLayout();
        if (stored < 0) {
            result = (int)stored;
            break;
        }
        done += n;

        if (done < size) {
            __i_dvc_ctrl.yield();
        }
    }

    pdiutil::safe_delete_array(block);
    return (result < 0) ? result : (int)done;
}
#endif

pdiutil::string DevFs::basename(const char* path) {
    if (!path) return pdiutil::string();
    const char* last = path;
    for (const char* p = path; *p; ++p) {
        if (*p == PATH_SEPARATOR_CHAR) last = p + 1;
    }
    return pdiutil::string(last);
}

int DevFs::streamFill(bool random, uint64_t size, pdiutil::function<bool(char*, uint32_t)> readbackfn, uint64_t offset, const char* readUntilMatchStr, bool* didmatchfound) {
    // Unbounded nodes are capped so `cat` terminates on an MCU.
    uint32_t cap = DEVFS_STREAM_READ_MAX;
    if (offset >= cap) return 0;

    // the whole capped stream is produced before it is delivered, so a match is
    // seen even where it falls across what would otherwise be two chunks
    char stream[DEVFS_STREAM_READ_MAX];
    uint32_t total = cap - (uint32_t)offset;

    if (random) {
        uint32_t i = 0;
        while (i < total) {
            uint32_t r = __i_dvc_ctrl.random_now();
            uint32_t take = (total - i) < 4 ? (total - i) : 4;
            memcpy(stream + i, &r, take);
            i += take;
        }
    } else {
        memset(stream, 0, total);
    }

    if (nullptr != readUntilMatchStr && '\0' != readUntilMatchStr[0]) {
        int32_t at = __strstr(stream, total, readUntilMatchStr,
                              (uint32_t)strlen(readUntilMatchStr), 0);
        if (at >= 0) {
            total = (uint32_t)at;
            if (nullptr != didmatchfound) *didmatchfound = true;
            if (0 == total) return 0;
        }
    }

    uint32_t chunk = (size > 0 && size < total) ? (uint32_t)size : total;
    uint32_t done = 0;
    while (done < total) {
        uint32_t n = total - done;
        if (n > chunk) n = chunk;
        if (!readbackfn(stream + done, n)) break;
        done += n;
    }
    return (int)done;
}

int DevFs::readFile(const char* path, uint64_t size, pdiutil::function<bool(char*, uint32_t)> readbackfn, uint64_t offset, const char* readUntilMatchStr, bool* didmatchfound) {
    if (!path || !readbackfn) return PDI_ERR_INVALID_ARG;
    switch (classify(path)) {
        case DEV_NULL:    return 0; // EOF, deliver nothing
        case DEV_ZERO:    return streamFill(false, size, readbackfn, offset, readUntilMatchStr, didmatchfound);
        case DEV_RANDOM:
        case DEV_URANDOM: return streamFill(true, size, readbackfn, offset, readUntilMatchStr, didmatchfound);
#ifdef ENABLE_DRIVER_CORE
        case DEV_DRIVER_NODE:
        case DEV_PARTITION: {
            int16_t device = -1;
            uint32_t first = 0, span = 0;
            if (diskWindow(path, device, first, span)) {
                return diskRead(device, first, span, size, readbackfn, offset, readUntilMatchStr, didmatchfound);
            }
            bool isdir = false;
            return driverRead(driverNode(normalizePath(path), isdir), size, readbackfn, offset);
        }
#endif
        default:          return PDI_ERR_NOT_FOUND;
    }
}

int DevFs::writeFile(const char* path, const char* content, uint32_t size, bool append) {
    // Every writable dev node discards its input; report success.
    NodeKind k = classify(path);
    if (k == DEV_ROOT || k == DEV_DRIVER_DIR) return STORAGE_ERROR_NOT_A_FILE;
    if (k == DEV_INVALID) return PDI_ERR_NOT_FOUND;
#ifdef ENABLE_DRIVER_CORE
    if ((k == DEV_DRIVER_NODE || k == DEV_PARTITION) && isDisk(path)) {
        return append ? PDI_ERR_NOT_SUPPORTED : editFile(path, 0, content, size);
    }
#endif
    if (k == DEV_DRIVER_NODE) return STORAGE_ERROR_READ_ONLY;
    return (int)size;
}

/**
 * Stores bytes in a disk from the offset; a discarding node takes any edit, and a
 * driver's node takes none.
 */
int DevFs::editFile(const char* path, uint64_t offset, const char* content, uint32_t size) {
#ifdef ENABLE_DRIVER_CORE
    int16_t device = -1;
    uint32_t first = 0, span = 0;
    if (diskWindow(path, device, first, span)) {
        return diskWrite(device, first, span, offset, content, size);
    }
#endif
    NodeKind k = classify(path);
    if (k == DEV_ROOT || k == DEV_DRIVER_DIR) return STORAGE_ERROR_NOT_A_FILE;
    if (k == DEV_INVALID) return PDI_ERR_NOT_FOUND;
    if (k == DEV_DRIVER_NODE || k == DEV_PARTITION) return PDI_ERR_NOT_SUPPORTED;
    return (int)size;
}

int64_t DevFs::getFileSize(const char* path) {
    NodeKind k = classify(path);
    if (k == DEV_ROOT || k == DEV_DRIVER_DIR || k == DEV_INVALID) return STORAGE_ERROR_NOT_A_FILE;
#ifdef ENABLE_DRIVER_CORE
    if (k == DEV_DRIVER_NODE || k == DEV_PARTITION) return (int64_t)diskSize(path);
#endif
    return 0;
}

bool DevFs::isFileExist(const char* path) {
    NodeKind k = classify(path);
    return (k != DEV_ROOT && k != DEV_DRIVER_DIR && k != DEV_INVALID);
}

bool DevFs::isDirExist(const char* path) {
    NodeKind k = classify(path);
    return k == DEV_ROOT || k == DEV_DRIVER_DIR;
}

bool DevFs::isDirectory(const char* path) {
    return isDirExist(path);
}

int DevFs::getDirFileList(const char* path, pdiutil::vector<file_info_t>& items, const char* pattern) {
    NodeKind k = classify(path);
#ifdef ENABLE_DRIVER_CORE
    if (k == DEV_DRIVER_DIR) {
        listDriverNodes(normalizePath(path), items);
        return (int)items.size();
    }
#endif
    if (k != DEV_ROOT) return STORAGE_ERROR_NOT_A_DIRECTORY;

    for (uint8_t i = 0; i < s_dev_file_count; ++i) {
        file_info_t info;
        memset(&info, 0, sizeof(info));
        info.m_type = FILE_TYPE_REG;
        info.m_size = 0;
        // Callers (ls) delete[] m_name — allocate a heap copy of the literal.
        uint32_t nlen = strlen(s_dev_files[i]);
        info.m_name = pdiutil::safe_new_array<char>(nlen + 1);
        if (nullptr == info.m_name) continue;
        memcpy(info.m_name, s_dev_files[i], nlen);
        info.m_name[nlen] = '\0';
        info.m_perms = 0666;
        info.m_uid = 0;
        info.m_gid = 0;
        info.m_ctime = 0;
        info.m_mtime = 0;
        items.push_back(info);
    }
#ifdef ENABLE_DRIVER_CORE
    listDriverNodes("", items);
#endif
    return (int)items.size();
}

int DevFs::getFileAttr(const char* path, uint8_t type, void* buffer, uint32_t size) {
    if (!buffer || size == 0) return PDI_ERR_INVALID_ARG;
    NodeKind k = classify(path);
    if (k == DEV_INVALID) return PDI_ERR_NOT_FOUND;

    if (type == FILE_ATTR_PERMS && size >= sizeof(uint16_t)) {
        *(uint16_t*)buffer = (k == DEV_ROOT || k == DEV_DRIVER_DIR) ? 0555 : (k == DEV_DRIVER_NODE) ? 0444 : 0666;
#ifdef ENABLE_DRIVER_CORE
        if ((k == DEV_DRIVER_NODE || k == DEV_PARTITION) && isDisk(path)) *(uint16_t*)buffer = 0600;
#endif
        return sizeof(uint16_t);
    }
    if (type == FILE_ATTR_UID && size >= sizeof(uint16_t)) {
        *(uint16_t*)buffer = 0;
        return sizeof(uint16_t);
    }
    if (type == FILE_ATTR_GID && size >= sizeof(uint16_t)) {
        *(uint16_t*)buffer = 0;
        return sizeof(uint16_t);
    }
    return STORAGE_ERROR_ATTR_NOT_FOUND;
}

pdi_err_t DevFs::getFileMeta(const char* path, file_info_t& out) {
    NodeKind k = classify(path);
    // Per iFileSystemInterface contract, m_name is left untouched.

    if (k == DEV_ROOT || k == DEV_DRIVER_DIR) {
        out.m_type  = FILE_TYPE_DIR;
        out.m_size  = 0;
        out.m_perms = 0555;
        out.m_uid   = 0;
        out.m_gid   = 0;
        out.m_ctime = 0;
        out.m_mtime = 0;
        return 0;
    }

    if (k != DEV_INVALID) {
        out.m_type  = FILE_TYPE_REG;
        out.m_size  = 0;
        out.m_perms = (k == DEV_DRIVER_NODE) ? 0444 : 0666;
#ifdef ENABLE_DRIVER_CORE
        if ((k == DEV_DRIVER_NODE || k == DEV_PARTITION) && isDisk(path)) {
            out.m_size = (int64_t)diskSize(path);
            out.m_perms = 0600;
        }
#endif
        out.m_uid   = 0;
        out.m_gid   = 0;
        out.m_ctime = 0;
        out.m_mtime = 0;
        return 0;
    }

    return PDI_ERR_NOT_FOUND;
}

#endif
