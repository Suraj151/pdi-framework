/********************************** ProcFS *************************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 21st July 2026
******************************************************************************/

#include <config/Config.h>

#ifdef ENABLE_PROCFS

#include "ProcFs.h"
#include <interface/pdi.h>

namespace {

class ProcFsNullStorage : public iStorageInterface {
public:
    int64_t read(uint64_t, void*, uint64_t) override { return -1; }
    int64_t write(uint64_t, const void*, uint64_t) override { return -1; }
    bool erase(uint64_t, uint64_t) override { return false; }
    uint64_t size() const override { return 0; }
};

ProcFsNullStorage s_proc_null_storage;

// Plain string literals at namespace scope — RODT_ATTR expands to a GCC
// statement-expression via F()/PSTR() on esp8266, which is only legal inside a
// function body. String literals here already live in RODATA/IROM.
const char* const s_proc_files[] = {
    "uptime",
    "version"
};

const uint8_t s_proc_file_count = sizeof(s_proc_files) / sizeof(s_proc_files[0]);

}

ProcFs __i_procfs;

ProcFs::ProcFs() : iFileSystemInterface(s_proc_null_storage) {}

const char* ProcFs::normalizePath(const char* path) const {
    if (!path) return "";
    while (*path == '/') path++;
    return path;
}

pdiutil::string ProcFs::basename(const char* path) {
    if (!path) return pdiutil::string();
    const char* last = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '/') last = p + 1;
    }
    return pdiutil::string(last);
}

pdiutil::string ProcFs::generateContent(const char* path) {
    const char* norm = normalizePath(path);
    char buf[128];
    buf[0] = '\0';

    if (strcmp_ro(norm, RODT_ATTR("uptime")) == 0) {
        uint32_t ms = __i_dvc_ctrl.millis_now();
        uint32_t sec = ms / 1000UL;
        uint32_t frac = (ms % 1000UL) / 10;
        pdiutil::string fmt = CHARPTR_WRAP("%u.%02u %u.%02u\n");
        __snprintf(buf, sizeof(buf), fmt.c_str(), sec, frac, sec, frac);
        return pdiutil::string(buf);
    }
    if (strcmp_ro(norm, RODT_ATTR("version")) == 0) {
        // RELEASE / CONFIG_VERSION land in IROM on esp8266; __vsnprintf's %s
        // reads char-by-char with plain *p and faults there. Marshal to RAM
        // first via CHARPTR_WRAP (same trick already used for format strings).
        pdiutil::string fmt = CHARPTR_WRAP("PDI Stack version %s (%s)\n");
        pdiutil::string rel = CHARPTR_WRAP(RELEASE);
        pdiutil::string cfg = CHARPTR_WRAP(CONFIG_VERSION);
        __snprintf(buf, sizeof(buf), fmt.c_str(), rel.c_str(), cfg.c_str());
        return pdiutil::string(buf);
    }
    return pdiutil::string();
}

int ProcFs::readFile(const char* path, uint64_t size, pdiutil::function<bool(char*, uint32_t)> readbackfn, uint64_t offset, const char* readUntilMatchStr, bool* didmatchfound) {
    if (!path || !readbackfn) return -1;
    pdiutil::string content = generateContent(path);
    if (content.empty()) return -1;
    if (offset >= content.length()) return 0;

    // `size` is the per-iteration chunk limit, not a total cap — loop the
    // callback until the whole content is delivered or it returns false.
    uint32_t total = content.length() - (uint32_t)offset;
    uint32_t chunk = (size > 0 && size < total) ? (uint32_t)size : total;
    uint32_t done = 0;
    while (done < total) {
        uint32_t n = total - done;
        if (n > chunk) n = chunk;
        if (!readbackfn((char*)content.c_str() + offset + done, n)) break;
        done += n;
    }
    return (int)done;
}

int64_t ProcFs::getFileSize(const char* path) {
    pdiutil::string content = generateContent(path);
    return content.empty() ? -1 : (int64_t)content.length();
}

bool ProcFs::isFileExist(const char* path) {
    const char* norm = normalizePath(path);
    for (uint8_t i = 0; i < s_proc_file_count; ++i) {
        if (strcmp(norm, s_proc_files[i]) == 0) return true;
    }
    return false;
}

bool ProcFs::isDirExist(const char* path) {
    const char* norm = normalizePath(path);
    return (norm[0] == '\0');
}

bool ProcFs::isDirectory(const char* path) {
    return isDirExist(path);
}

int ProcFs::getDirFileList(const char* path, pdiutil::vector<file_info_t>& items, const char* pattern) {
    const char* norm = normalizePath(path);
    if (norm[0] != '\0') return -1;

    for (uint8_t i = 0; i < s_proc_file_count; ++i) {
        file_info_t info;
        memset(&info, 0, sizeof(info));
        info.m_type = FILE_TYPE_REG;
        info.m_size = getFileSize(s_proc_files[i]);
        // Callers (ls) delete[] m_name — allocate a heap copy of the literal
        // so the free path is safe.
        uint32_t nlen = strlen(s_proc_files[i]);
        info.m_name = new char[nlen + 1];
        memcpy(info.m_name, s_proc_files[i], nlen);
        info.m_name[nlen] = '\0';
        info.m_perms = 0444;
        info.m_uid = 0;
        info.m_gid = 0;
        info.m_ctime = 0;
        info.m_mtime = 0;
        items.push_back(info);
    }
    return (int)items.size();
}

int ProcFs::getFileAttr(const char* path, uint8_t type, void* buffer, uint32_t size) {
    if (!buffer || size == 0) return -1;
    if (type == FILE_ATTR_PERMS && size >= sizeof(uint16_t)) {
        *(uint16_t*)buffer = 0444;
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

int ProcFs::getFileMeta(const char* path, file_info_t& out) {
    const char* norm = normalizePath(path);
    // Per iFileSystemInterface contract, m_name is left untouched. Do not
    // assign a static/IROM literal or a caller-borrowed pointer here — some
    // callers assume m_name (when set) is heap-owned and will delete[] it.

    if (norm[0] == '\0') {
        out.m_type  = FILE_TYPE_DIR;
        out.m_size  = 0;
        out.m_perms = 0555;
        out.m_uid   = 0;
        out.m_gid   = 0;
        out.m_ctime = 0;
        out.m_mtime = 0;
        return 0;
    }

    if (isFileExist(norm)) {
        out.m_type  = FILE_TYPE_REG;
        out.m_size  = getFileSize(norm);
        out.m_perms = 0444;
        out.m_uid   = 0;
        out.m_gid   = 0;
        out.m_ctime = 0;
        out.m_mtime = 0;
        return 0;
    }

    return -1;
}

#endif
