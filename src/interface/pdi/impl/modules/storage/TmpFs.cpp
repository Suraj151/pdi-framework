/********************************** TmpFS *************************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 23rd July 2026
******************************************************************************/

#include <config/Config.h>

#ifdef ENABLE_TMPFS

#include "TmpFs.h"
#include <interface/pdi/middlewares/iNtpInterface.h>
#include <service_provider/session/SessionManager.h>

namespace {

class TmpFsNullStorage : public iStorageInterface {
public:
    int64_t read(uint64_t, void*, uint64_t) override { return -1; }
    int64_t write(uint64_t, const void*, uint64_t) override { return -1; }
    bool erase(uint64_t, uint64_t) override { return false; }
    uint64_t size() const override { return 0; }
};

TmpFsNullStorage s_tmp_null_storage;

}

TmpFs __i_tmpfs;

TmpFs::TmpFs() : iFileSystemInterface(s_tmp_null_storage) {}

// ---- path helpers ---------------------------------------------------------

pdiutil::string TmpFs::normalize(const char* path) const {
    pdiutil::string out;
    if (!path) return out;
    while (*path == '/') path++;
    out = path;
    while (out.length() > 0 && out[out.length() - 1] == '/') {
        out.erase(out.length() - 1, 1);
    }
    return out;
}

int TmpFs::findNode(const pdiutil::string& norm) const {
    for (uint32_t i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].m_path == norm) return (int)i;
    }
    return -1;
}

pdiutil::string TmpFs::parentOf(const pdiutil::string& norm) const {
    int cut = -1;
    for (int i = (int)norm.length() - 1; i >= 0; --i) {
        if (norm[i] == '/') { cut = i; break; }
    }
    if (cut < 0) return pdiutil::string();
    return norm.substr(0, cut);
}

bool TmpFs::parentExists(const pdiutil::string& norm) const {
    pdiutil::string parent = parentOf(norm);
    if (parent.empty()) return true; // root always exists
    int idx = findNode(parent);
    return (idx >= 0 && m_nodes[idx].m_type == FILE_TYPE_DIR);
}

uint32_t TmpFs::usedBytes() const {
    uint32_t total = 0;
    for (uint32_t i = 0; i < m_nodes.size(); ++i) {
        total += (uint32_t)m_nodes[i].m_data.length();
    }
    return total;
}

// ---- stamping hooks -------------------------------------------------------

uint32_t TmpFs::nowEpoch() {
    if (!__i_ntp.is_valid_ntptime()) return 0;
    return (uint32_t)__i_ntp.get_ntp_time();
}

void TmpFs::currentOwner(uint16_t& uid, uint16_t& gid) {
#ifdef ENABLE_AUTH_SERVICE
    session_t* s = SessionManager::current();
    if (nullptr != s) { uid = s->m_uid; gid = s->m_gid; return; }
#endif
    uid = 0;
    gid = 0;
}

uint16_t TmpFs::currentUmask() {
    session_t* s = SessionManager::current();
    return (nullptr != s) ? s->m_umask : (uint16_t)FILE_UMASK_DEFAULT;
}

void TmpFs::stampNew(tmpfs_node_t& n, uint16_t defperms) {
    uint16_t uid = 0, gid = 0;
    currentOwner(uid, gid);
    n.m_uid = uid;
    n.m_gid = gid;
    n.m_perms = defperms & ~currentUmask();
    n.m_ctime = nowEpoch();
    n.m_mtime = n.m_ctime;
}

// ---- content writers ------------------------------------------------------

int TmpFs::putContent(const pdiutil::string& norm, const char* content, uint32_t size, bool append) {
    if (norm.empty() || norm.length() > TMPFS_MAX_PATH) return -1;

    int idx = findNode(norm);
    if (idx >= 0 && m_nodes[idx].m_type == FILE_TYPE_DIR) return -1;

    if (idx < 0) {
        if (!parentExists(norm)) return -1;
        if (m_nodes.size() >= TMPFS_MAX_NODES) return -1;
        if (usedBytes() + size > TMPFS_MAX_BYTES) return -1;
        tmpfs_node_t n;
        n.m_path = norm;
        n.m_type = FILE_TYPE_REG;
        stampNew(n, FILE_PERM_DEFAULT_FILE);
        if (content && size > 0) n.m_data.append(content, size);
        m_nodes.push_back(n);
        return (int)size;
    }

    tmpfs_node_t& node = m_nodes[idx];
    uint32_t oldlen = (uint32_t)node.m_data.length();
    uint32_t newused = append ? (usedBytes() + size) : (usedBytes() - oldlen + size);
    if (newused > TMPFS_MAX_BYTES) return -1;

    if (!append) node.m_data.clear();
    if (content && size > 0) node.m_data.append(content, size);
    node.m_mtime = nowEpoch();
    return (int)size;
}

// ---- write-side API -------------------------------------------------------

int TmpFs::createFile(const char* path, const char* content, int64_t size) {
    pdiutil::string norm = normalize(path);
    if (norm.empty()) return -1;
    if (findNode(norm) >= 0) return -1;
    uint32_t len = (size < 0) ? (content ? (uint32_t)strlen(content) : 0) : (uint32_t)size;
    return putContent(norm, content, len, false);
}

int TmpFs::writeFile(const char* path, const char* content, uint32_t size, bool append) {
    pdiutil::string norm = normalize(path);
    return putContent(norm, content, size, append);
}

int TmpFs::editFile(const char* path, uint64_t offset, const char* content, uint32_t size) {
    pdiutil::string norm = normalize(path);
    int idx = findNode(norm);
    if (idx < 0 || m_nodes[idx].m_type != FILE_TYPE_REG || !content) return -1;

    tmpfs_node_t& node = m_nodes[idx];
    uint32_t oldlen = (uint32_t)node.m_data.length();
    uint32_t end = (uint32_t)offset + size;
    if (end > oldlen) {
        if (usedBytes() - oldlen + end > TMPFS_MAX_BYTES) return -1;
        node.m_data.resize(end, '\0');
    }
    for (uint32_t i = 0; i < size; ++i) {
        node.m_data[(uint32_t)offset + i] = content[i];
    }
    node.m_mtime = nowEpoch();
    return (int)size;
}

int TmpFs::readFile(const char* path, uint64_t size, pdiutil::function<bool(char*, uint32_t)> readbackfn, uint64_t offset, const char* readUntilMatchStr, bool* didmatchfound) {
    if (!path || !readbackfn) return -1;
    pdiutil::string norm = normalize(path);
    int idx = findNode(norm);
    if (idx < 0 || m_nodes[idx].m_type != FILE_TYPE_REG) return -1;

    pdiutil::string& data = m_nodes[idx].m_data;
    if (offset >= data.length()) return 0;

    // `size` is the per-iteration chunk limit — loop until content is drained.
    uint32_t total = (uint32_t)data.length() - (uint32_t)offset;
    uint32_t chunk = (size > 0 && size < total) ? (uint32_t)size : total;
    uint32_t done = 0;
    while (done < total) {
        uint32_t n = total - done;
        if (n > chunk) n = chunk;
        if (!readbackfn((char*)data.c_str() + offset + done, n)) break;
        done += n;
    }
    return (int)done;
}

int TmpFs::createDirectory(const char* path) {
    pdiutil::string norm = normalize(path);
    if (norm.empty() || norm.length() > TMPFS_MAX_PATH) return -1;
    if (findNode(norm) >= 0) return -1;
    if (!parentExists(norm)) return -1;
    if (m_nodes.size() >= TMPFS_MAX_NODES) return -1;

    tmpfs_node_t n;
    n.m_path = norm;
    n.m_type = FILE_TYPE_DIR;
    stampNew(n, FILE_PERM_DEFAULT_DIR);
    m_nodes.push_back(n);
    return 0;
}

int TmpFs::deleteFile(const char* path) {
    pdiutil::string norm = normalize(path);
    int idx = findNode(norm);
    if (idx < 0 || m_nodes[idx].m_type != FILE_TYPE_REG) return -1;
    m_nodes.erase(m_nodes.begin() + idx);
    return 0;
}

int TmpFs::deleteDirectory(const char* path) {
    pdiutil::string norm = normalize(path);
    if (norm.empty()) return -1;
    int idx = findNode(norm);
    if (idx < 0 || m_nodes[idx].m_type != FILE_TYPE_DIR) return -1;

    // Remove the directory and every descendant (prefix "norm/").
    pdiutil::string prefix = norm;
    prefix += "/";
    for (int i = (int)m_nodes.size() - 1; i >= 0; --i) {
        const pdiutil::string& p = m_nodes[i].m_path;
        if (p == norm || (p.length() > prefix.length() && p.compare(0, prefix.length(), prefix) == 0)) {
            m_nodes.erase(m_nodes.begin() + i);
        }
    }
    return 0;
}

int TmpFs::rename(const char* oldPath, const char* newPath) {
    pdiutil::string oldn = normalize(oldPath);
    pdiutil::string newn = normalize(newPath);
    if (oldn.empty() || newn.empty()) return -1;
    int oidx = findNode(oldn);
    if (oidx < 0) return -1;
    if (findNode(newn) >= 0) return -1;
    if (!parentExists(newn)) return -1;
    if (newn.length() > TMPFS_MAX_PATH) return -1;

    bool isdir = (m_nodes[oidx].m_type == FILE_TYPE_DIR);
    m_nodes[oidx].m_path = newn;

    if (isdir) {
        pdiutil::string oldprefix = oldn; oldprefix += "/";
        pdiutil::string newprefix = newn; newprefix += "/";
        for (uint32_t i = 0; i < m_nodes.size(); ++i) {
            pdiutil::string& p = m_nodes[i].m_path;
            if (p.length() > oldprefix.length() && p.compare(0, oldprefix.length(), oldprefix) == 0) {
                p = newprefix + p.substr(oldprefix.length());
            }
        }
    }
    return 0;
}

int TmpFs::moveFile(const char* oldPath, const char* newPath) {
    return rename(oldPath, newPath);
}

int TmpFs::copyFile(const char* sourcePath, const char* destPath) {
    pdiutil::string src = normalize(sourcePath);
    pdiutil::string dst = normalize(destPath);
    int sidx = findNode(src);
    if (sidx < 0 || m_nodes[sidx].m_type != FILE_TYPE_REG) return -1;
    if (dst.empty() || findNode(dst) >= 0) return -1;
    if (!parentExists(dst) || dst.length() > TMPFS_MAX_PATH) return -1;
    if (m_nodes.size() >= TMPFS_MAX_NODES) return -1;
    uint32_t len = (uint32_t)m_nodes[sidx].m_data.length();
    if (usedBytes() + len > TMPFS_MAX_BYTES) return -1;

    tmpfs_node_t n;
    n.m_path = dst;
    n.m_type = FILE_TYPE_REG;
    n.m_data = m_nodes[sidx].m_data;
    stampNew(n, FILE_PERM_DEFAULT_FILE);
    m_nodes.push_back(n);
    return 0;
}

int TmpFs::touch(const char* path) {
    pdiutil::string norm = normalize(path);
    int idx = findNode(norm);
    if (idx >= 0) {
        m_nodes[idx].m_mtime = nowEpoch();
        return 0;
    }
    return createFile(path, nullptr, 0);
}

// ---- query API ------------------------------------------------------------

int64_t TmpFs::getFileSize(const char* path) {
    pdiutil::string norm = normalize(path);
    int idx = findNode(norm);
    if (idx < 0 || m_nodes[idx].m_type != FILE_TYPE_REG) return -1;
    return (int64_t)m_nodes[idx].m_data.length();
}

bool TmpFs::isFileExist(const char* path) {
    pdiutil::string norm = normalize(path);
    int idx = findNode(norm);
    return (idx >= 0 && m_nodes[idx].m_type == FILE_TYPE_REG);
}

bool TmpFs::isDirExist(const char* path) {
    pdiutil::string norm = normalize(path);
    if (norm.empty()) return true; // root
    int idx = findNode(norm);
    return (idx >= 0 && m_nodes[idx].m_type == FILE_TYPE_DIR);
}

bool TmpFs::isDirectory(const char* path) {
    return isDirExist(path);
}

int TmpFs::getDirFileList(const char* path, pdiutil::vector<file_info_t>& items, const char* pattern) {
    pdiutil::string norm = normalize(path);
    if (!isDirExist(path)) return -1;

    for (uint32_t i = 0; i < m_nodes.size(); ++i) {
        const tmpfs_node_t& node = m_nodes[i];
        if (parentOf(node.m_path) != norm) continue;

        pdiutil::string base = basename(node.m_path.c_str());
        file_info_t info;
        memset(&info, 0, sizeof(info));
        info.m_type  = node.m_type;
        info.m_size  = (node.m_type == FILE_TYPE_REG) ? (int64_t)node.m_data.length() : 0;
        info.m_perms = node.m_perms;
        info.m_uid   = node.m_uid;
        info.m_gid   = node.m_gid;
        info.m_ctime = node.m_ctime;
        info.m_mtime = node.m_mtime;
        // Callers (ls) delete[] m_name — hand them a heap copy.
        uint32_t nlen = (uint32_t)base.length();
        info.m_name = new char[nlen + 1];
        memcpy(info.m_name, base.c_str(), nlen);
        info.m_name[nlen] = '\0';
        items.push_back(info);
    }
    return (int)items.size();
}

pdiutil::string TmpFs::basename(const char* path) {
    if (!path) return pdiutil::string();
    const char* last = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '/') last = p + 1;
    }
    return pdiutil::string(last);
}

// ---- attribute API --------------------------------------------------------

int TmpFs::getFileAttr(const char* path, uint8_t type, void* buffer, uint32_t size) {
    if (!buffer || size == 0) return -1;
    pdiutil::string norm = normalize(path);

    uint16_t perms, uid, gid;
    uint32_t ctime, mtime;
    if (norm.empty()) {
        perms = 0777; uid = 0; gid = 0; ctime = 0; mtime = 0;
    } else {
        int idx = findNode(norm);
        if (idx < 0) return -1;
        perms = m_nodes[idx].m_perms; uid = m_nodes[idx].m_uid; gid = m_nodes[idx].m_gid;
        ctime = m_nodes[idx].m_ctime; mtime = m_nodes[idx].m_mtime;
    }

    if (type == FILE_ATTR_PERMS && size >= sizeof(uint16_t)) { *(uint16_t*)buffer = perms; return sizeof(uint16_t); }
    if (type == FILE_ATTR_UID && size >= sizeof(uint16_t))   { *(uint16_t*)buffer = uid;   return sizeof(uint16_t); }
    if (type == FILE_ATTR_GID && size >= sizeof(uint16_t))   { *(uint16_t*)buffer = gid;   return sizeof(uint16_t); }
    if (type == FILE_ATTR_CTIME && size >= sizeof(uint32_t)) { *(uint32_t*)buffer = ctime; return sizeof(uint32_t); }
    if (type == FILE_ATTR_MTIME && size >= sizeof(uint32_t)) { *(uint32_t*)buffer = mtime; return sizeof(uint32_t); }
    return -1;
}

int TmpFs::setFileAttr(const char* path, uint8_t type, const void* buffer, uint32_t size) {
    if (!buffer || size == 0) return -1;
    pdiutil::string norm = normalize(path);
    int idx = findNode(norm);
    if (idx < 0) return -1;
    tmpfs_node_t& node = m_nodes[idx];

    if (type == FILE_ATTR_PERMS && size >= sizeof(uint16_t)) { node.m_perms = *(const uint16_t*)buffer; return (int)size; }
    if (type == FILE_ATTR_UID && size >= sizeof(uint16_t))   { node.m_uid   = *(const uint16_t*)buffer; return (int)size; }
    if (type == FILE_ATTR_GID && size >= sizeof(uint16_t))   { node.m_gid   = *(const uint16_t*)buffer; return (int)size; }
    if (type == FILE_ATTR_CTIME && size >= sizeof(uint32_t)) { node.m_ctime = *(const uint32_t*)buffer; return (int)size; }
    if (type == FILE_ATTR_MTIME && size >= sizeof(uint32_t)) { node.m_mtime = *(const uint32_t*)buffer; return (int)size; }
    return -1;
}

int TmpFs::getFileMeta(const char* path, file_info_t& out) {
    pdiutil::string norm = normalize(path);
    // m_name is left untouched per the interface contract.

    if (norm.empty()) {
        out.m_type = FILE_TYPE_DIR; out.m_size = 0; out.m_perms = 0777;
        out.m_uid = 0; out.m_gid = 0; out.m_ctime = 0; out.m_mtime = 0;
        return 0;
    }

    int idx = findNode(norm);
    if (idx < 0) return -1;
    const tmpfs_node_t& node = m_nodes[idx];
    out.m_type  = node.m_type;
    out.m_size  = (node.m_type == FILE_TYPE_REG) ? (int64_t)node.m_data.length() : 0;
    out.m_perms = node.m_perms;
    out.m_uid   = node.m_uid;
    out.m_gid   = node.m_gid;
    out.m_ctime = node.m_ctime;
    out.m_mtime = node.m_mtime;
    return 0;
}

int TmpFs::setFilePermissions(const char* path, uint16_t perms) {
    pdiutil::string norm = normalize(path);
    int idx = findNode(norm);
    if (idx < 0) return -1;
    m_nodes[idx].m_perms = perms;
    return 0;
}

int TmpFs::setFileOwner(const char* path, uint16_t uid, uint16_t gid) {
    pdiutil::string norm = normalize(path);
    int idx = findNode(norm);
    if (idx < 0) return -1;
    m_nodes[idx].m_uid = uid;
    m_nodes[idx].m_gid = gid;
    return 0;
}

#endif
