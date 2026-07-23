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

namespace {

class DevFsNullStorage : public iStorageInterface {
public:
    int64_t read(uint64_t, void*, uint64_t) override { return -1; }
    int64_t write(uint64_t, const void*, uint64_t) override { return -1; }
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

const char* DevFs::normalizePath(const char* path) const {
    if (!path) return "";
    while (*path == '/') path++;
    return path;
}

DevFs::NodeKind DevFs::classify(const char* path) const {
    const char* p = normalizePath(path);
    if (p[0] == '\0') return DEV_ROOT;
    if (strcmp(p, "null") == 0) return DEV_NULL;
    if (strcmp(p, "zero") == 0) return DEV_ZERO;
    if (strcmp(p, "random") == 0) return DEV_RANDOM;
    if (strcmp(p, "urandom") == 0) return DEV_URANDOM;
    return DEV_INVALID;
}

pdiutil::string DevFs::basename(const char* path) {
    if (!path) return pdiutil::string();
    const char* last = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '/') last = p + 1;
    }
    return pdiutil::string(last);
}

int DevFs::streamFill(bool random, uint64_t size, pdiutil::function<bool(char*, uint32_t)> readbackfn, uint64_t offset) {
    // Unbounded nodes are capped so `cat` terminates on an MCU.
    uint32_t cap = DEVFS_STREAM_READ_MAX;
    if (offset >= cap) return 0;

    uint32_t total = cap - (uint32_t)offset;
    uint32_t chunk = (size > 0 && size < total) ? (uint32_t)size : total;
    char buf[32];
    uint32_t done = 0;
    while (done < total) {
        uint32_t n = total - done;
        if (n > chunk) n = chunk;
        if (n > sizeof(buf)) n = sizeof(buf);
        if (random) {
            uint32_t i = 0;
            while (i < n) {
                uint32_t r = __i_dvc_ctrl.random_now();
                uint32_t take = (n - i) < 4 ? (n - i) : 4;
                memcpy(buf + i, &r, take);
                i += take;
            }
        } else {
            memset(buf, 0, n);
        }
        if (!readbackfn(buf, n)) break;
        done += n;
    }
    return (int)done;
}

int DevFs::readFile(const char* path, uint64_t size, pdiutil::function<bool(char*, uint32_t)> readbackfn, uint64_t offset, const char* readUntilMatchStr, bool* didmatchfound) {
    if (!path || !readbackfn) return -1;
    switch (classify(path)) {
        case DEV_NULL:    return 0; // EOF, deliver nothing
        case DEV_ZERO:    return streamFill(false, size, readbackfn, offset);
        case DEV_RANDOM:
        case DEV_URANDOM: return streamFill(true, size, readbackfn, offset);
        default:          return -1;
    }
}

int DevFs::writeFile(const char* path, const char* content, uint32_t size, bool append) {
    // Every writable dev node discards its input; report success.
    NodeKind k = classify(path);
    if (k == DEV_ROOT || k == DEV_INVALID) return -1;
    return (int)size;
}

int64_t DevFs::getFileSize(const char* path) {
    NodeKind k = classify(path);
    return (k == DEV_ROOT || k == DEV_INVALID) ? -1 : 0;
}

bool DevFs::isFileExist(const char* path) {
    NodeKind k = classify(path);
    return (k != DEV_ROOT && k != DEV_INVALID);
}

bool DevFs::isDirExist(const char* path) {
    return classify(path) == DEV_ROOT;
}

bool DevFs::isDirectory(const char* path) {
    return isDirExist(path);
}

int DevFs::getDirFileList(const char* path, pdiutil::vector<file_info_t>& items, const char* pattern) {
    if (classify(path) != DEV_ROOT) return -1;

    for (uint8_t i = 0; i < s_dev_file_count; ++i) {
        file_info_t info;
        memset(&info, 0, sizeof(info));
        info.m_type = FILE_TYPE_REG;
        info.m_size = 0;
        // Callers (ls) delete[] m_name — allocate a heap copy of the literal.
        uint32_t nlen = strlen(s_dev_files[i]);
        info.m_name = new char[nlen + 1];
        memcpy(info.m_name, s_dev_files[i], nlen);
        info.m_name[nlen] = '\0';
        info.m_perms = 0666;
        info.m_uid = 0;
        info.m_gid = 0;
        info.m_ctime = 0;
        info.m_mtime = 0;
        items.push_back(info);
    }
    return (int)items.size();
}

int DevFs::getFileAttr(const char* path, uint8_t type, void* buffer, uint32_t size) {
    if (!buffer || size == 0) return -1;
    NodeKind k = classify(path);
    if (k == DEV_INVALID) return -1;

    if (type == FILE_ATTR_PERMS && size >= sizeof(uint16_t)) {
        *(uint16_t*)buffer = (k == DEV_ROOT) ? 0555 : 0666;
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
    return -1;
}

int DevFs::getFileMeta(const char* path, file_info_t& out) {
    NodeKind k = classify(path);
    // Per iFileSystemInterface contract, m_name is left untouched.

    if (k == DEV_ROOT) {
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
        out.m_perms = 0666;
        out.m_uid   = 0;
        out.m_gid   = 0;
        out.m_ctime = 0;
        out.m_mtime = 0;
        return 0;
    }

    return -1;
}

#endif
