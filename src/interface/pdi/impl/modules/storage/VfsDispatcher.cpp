/****************************** VFS Dispatcher ********************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 20th July 2026
******************************************************************************/

#include <config/Config.h>

#ifdef ENABLE_STORAGE_SERVICE

#include "VfsDispatcher.h"

#ifdef ENABLE_AUTH_SERVICE
#include <service_provider/session/SessionManager.h>
#endif

namespace {

class VfsNullStorage : public iStorageInterface {
public:
    int64_t read(uint64_t, void*, uint64_t) override { return -1; }
    int64_t write(uint64_t, const void*, uint64_t) override { return -1; }
    bool erase(uint64_t, uint64_t) override { return false; }
    uint64_t size() const override { return 0; }
};

VfsNullStorage s_null_storage;

const char* s_root_path = "/";
const pdiutil::string s_empty_string;

}

VfsDispatcher::VfsDispatcher() : iFileSystemInterface(s_null_storage), m_mount_count(0), m_priv_depth(0) {}

int8_t VfsDispatcher::mount(const char* prefix, iFileSystemInterface* backend, const char* name, vfs_type_t type) {
    if (!prefix || !backend || m_mount_count >= VFS_MAX_MOUNTS) {
        return -1;
    }
    uint32_t plen = strlen(prefix);
    if (plen == 0 || plen > VFS_MOUNT_PREFIX_MAX) {
        return -1;
    }
    for (uint8_t i = 0; i < m_mount_count; ++i) {
        if (strcmp(m_mounts[i].m_prefix, prefix) == 0) {
            return -1;
        }
    }

    vfs_mount_t& m = m_mounts[m_mount_count];
    memset(m.m_prefix, 0, sizeof(m.m_prefix));
    memcpy(m.m_prefix, prefix, plen);
    m.m_prefix_len = (uint8_t)plen;

    memset(m.m_name, 0, sizeof(m.m_name));
    if (name) {
        uint32_t nlen = strlen(name);
        if (nlen > VFS_MOUNT_NAME_MAX) nlen = VFS_MOUNT_NAME_MAX;
        memcpy(m.m_name, name, nlen);
    }

    m.m_backend = backend;
    m.m_type = type;
    ++m_mount_count;
    return 0;
}

iFileSystemInterface* VfsDispatcher::resolve(const char* path, const char** relpath_out) const {
    if (!path) {
        if (relpath_out) *relpath_out = nullptr;
        return rootBackend();
    }

    uint8_t best_idx = 0xFF;
    uint8_t best_len = 0;

    for (uint8_t i = 0; i < m_mount_count; ++i) {
        const vfs_mount_t& m = m_mounts[i];
        uint8_t plen = m.m_prefix_len;
        if (plen == 0) continue;

        if (plen == 1 && m.m_prefix[0] == '/') {
            if (best_len == 0) { best_idx = i; best_len = 1; }
            continue;
        }
        if (strncmp(path, m.m_prefix, plen) != 0) continue;
        char after = path[plen];
        if (after != '\0' && after != '/') continue;
        if (plen > best_len) { best_idx = i; best_len = plen; }
    }

    if (best_idx == 0xFF) {
        if (relpath_out) *relpath_out = path;
        return nullptr;
    }

    if (relpath_out) {
        if (m_mounts[best_idx].m_prefix_len == 1 && m_mounts[best_idx].m_prefix[0] == '/') {
            *relpath_out = path;
        } else {
            const char* suffix = path + best_len;
            *relpath_out = (*suffix == '\0') ? s_root_path : suffix;
        }
    }
    return m_mounts[best_idx].m_backend;
}

iFileSystemInterface* VfsDispatcher::rootBackend() const {
    for (uint8_t i = 0; i < m_mount_count; ++i) {
        if (m_mounts[i].m_prefix_len == 1 && m_mounts[i].m_prefix[0] == '/') {
            return m_mounts[i].m_backend;
        }
    }
    return nullptr;
}

const vfs_mount_t* VfsDispatcher::findMountForPath(const char* path) const {
    if (!path) return nullptr;
    uint8_t best_idx = 0xFF;
    uint8_t best_len = 0;
    for (uint8_t i = 0; i < m_mount_count; ++i) {
        const vfs_mount_t& m = m_mounts[i];
        uint8_t plen = m.m_prefix_len;
        if (plen == 0) continue;
        if (plen == 1 && m.m_prefix[0] == '/') {
            if (best_len == 0) { best_idx = i; best_len = 1; }
            continue;
        }
        if (strncmp(path, m.m_prefix, plen) != 0) continue;
        char after = path[plen];
        if (after != '\0' && after != '/') continue;
        if (plen > best_len) { best_idx = i; best_len = plen; }
    }
    return (best_idx == 0xFF) ? nullptr : &m_mounts[best_idx];
}

int VfsDispatcher::init() {
    int rc = 0;
    for (uint8_t i = 0; i < m_mount_count; ++i) {
        if (m_mounts[i].m_backend) {
            int r = m_mounts[i].m_backend->init();
            if (r != 0 && rc == 0) rc = r;
        }
    }
    return rc;
}

bool VfsDispatcher::checkRoot() {
    if (isPrivileged()) return true;
#ifdef ENABLE_AUTH_SERVICE
    return SessionManager::getCurrentUid() == 0;
#else
    return true;
#endif
}

bool VfsDispatcher::checkAccess(const char* path, uint8_t need_mask) {
    if (isPrivileged()) return true;
#ifdef ENABLE_AUTH_SERVICE
    uint16_t uid = SessionManager::getCurrentUid();
    if (uid == 0) return true;

    file_info_t meta;
    if (getFileMeta(path, meta) != 0) return true;

    uint8_t bits;
    if (meta.m_uid == uid) {
        bits = (meta.m_perms >> 6) & 7;
    } else if (meta.m_gid == SessionManager::getCurrentGid()) {
        bits = (meta.m_perms >> 3) & 7;
    } else {
        bits = meta.m_perms & 7;
    }
    return (bits & need_mask) == need_mask;
#else
    (void)path; (void)need_mask;
    return true;
#endif
}

bool VfsDispatcher::checkOwnerOrRoot(const char* path) {
    if (isPrivileged()) return true;
#ifdef ENABLE_AUTH_SERVICE
    uint16_t uid = SessionManager::getCurrentUid();
    if (uid == 0) return true;

    file_info_t meta;
    if (getFileMeta(path, meta) != 0) return true;
    return meta.m_uid == uid;
#else
    (void)path;
    return true;
#endif
}

#define VFS_ROUTE_PATH(_method, _path, _fail, ...)                                  \
    do {                                                                            \
        const char* rel = nullptr;                                                  \
        iFileSystemInterface* b = resolve((_path), &rel);                           \
        if (!b) return (_fail);                                                     \
        return b->_method(rel, ##__VA_ARGS__);                                      \
    } while (0)

int VfsDispatcher::createFile(const char* path, const char* content, int64_t size) {
    VFS_ROUTE_PATH(createFile, path, -1, content, size);
}
int VfsDispatcher::editFile(const char* path, uint64_t offset, const char* content, uint32_t size) {
    if (!checkAccess(path, VFS_ACCESS_W)) return -1;
    VFS_ROUTE_PATH(editFile, path, -1, offset, content, size);
}
int VfsDispatcher::writeFile(const char* path, const char* content, uint32_t size, bool append) {
    if (!checkAccess(path, VFS_ACCESS_W)) return -1;
    VFS_ROUTE_PATH(writeFile, path, -1, content, size, append);
}
int VfsDispatcher::readFile(const char* path, uint64_t size, pdiutil::function<bool(char*, uint32_t)> readbackfn, uint64_t offset, const char* readUntilMatchStr, bool* didmatchfound) {
    if (!checkAccess(path, VFS_ACCESS_R)) return -1;
    VFS_ROUTE_PATH(readFile, path, -1, size, readbackfn, offset, readUntilMatchStr, didmatchfound);
}
int64_t VfsDispatcher::getOffsetFromLineNumber(const char* path, int linenumber, CallBackVoidArgFn yield) {
    if (!checkAccess(path, VFS_ACCESS_R)) return -1;
    VFS_ROUTE_PATH(getOffsetFromLineNumber, path, -1, linenumber, yield);
}
int64_t VfsDispatcher::getLineNumberFromOffset(const char* path, int64_t offset, CallBackVoidArgFn yield) {
    if (!checkAccess(path, VFS_ACCESS_R)) return -1;
    VFS_ROUTE_PATH(getLineNumberFromOffset, path, -1, offset, yield);
}
int VfsDispatcher::findInFile(const char* path, const char* findStr, pdiutil::vector<uint32_t>* findindices, int maxindices, int everynthindice, int64_t offset, CallBackVoidArgFn yield) {
    if (!checkAccess(path, VFS_ACCESS_R)) return -1;
    VFS_ROUTE_PATH(findInFile, path, -1, findStr, findindices, maxindices, everynthindice, offset, yield);
}
int VfsDispatcher::getLineNumbersInFile(const char* path, pdiutil::vector<uint32_t>& linenumberindices, int maxlinenumbers, int linenumberoffset, CallBackVoidArgFn yield) {
    if (!checkAccess(path, VFS_ACCESS_R)) return -1;
    VFS_ROUTE_PATH(getLineNumbersInFile, path, -1, linenumberindices, maxlinenumbers, linenumberoffset, yield);
}
int VfsDispatcher::readLineInFile(const char* path, int32_t linenumber, pdiutil::string& linedata, const char* pattern, CallBackVoidArgFn yield) {
    if (!checkAccess(path, VFS_ACCESS_R)) return -1;
    VFS_ROUTE_PATH(readLineInFile, path, -1, linenumber, linedata, pattern, yield);
}
int VfsDispatcher::createDirectory(const char* path) {
    VFS_ROUTE_PATH(createDirectory, path, -1);
}
int VfsDispatcher::deleteDirectory(const char* path) {
    if (!checkAccess(path, VFS_ACCESS_W)) return -1;
    VFS_ROUTE_PATH(deleteDirectory, path, -1);
}
int VfsDispatcher::deleteFile(const char* path) {
    if (!checkAccess(path, VFS_ACCESS_W)) return -1;
    VFS_ROUTE_PATH(deleteFile, path, -1);
}
int64_t VfsDispatcher::getFileSize(const char* path) {
    VFS_ROUTE_PATH(getFileSize, path, -1);
}
int VfsDispatcher::getDirFileList(const char* path, pdiutil::vector<file_info_t>& items, const char* pattern) {
    if (!checkAccess(path, VFS_ACCESS_R)) return -1;
    VFS_ROUTE_PATH(getDirFileList, path, -1, items, pattern);
}
bool VfsDispatcher::isFileExist(const char* path) {
    const char* rel = nullptr;
    iFileSystemInterface* b = resolve(path, &rel);
    return b ? b->isFileExist(rel) : false;
}
bool VfsDispatcher::isDirExist(const char* path) {
    const char* rel = nullptr;
    iFileSystemInterface* b = resolve(path, &rel);
    return b ? b->isDirExist(rel) : false;
}
bool VfsDispatcher::isDirectory(const char* path) {
    const char* rel = nullptr;
    iFileSystemInterface* b = resolve(path, &rel);
    return b ? b->isDirectory(rel) : false;
}
int VfsDispatcher::setFileAttr(const char* path, uint8_t type, const void* buffer, uint32_t size) {
    if (!checkOwnerOrRoot(path)) return -1;
    VFS_ROUTE_PATH(setFileAttr, path, -1, type, buffer, size);
}
int VfsDispatcher::getFileAttr(const char* path, uint8_t type, void* buffer, uint32_t size) {
    VFS_ROUTE_PATH(getFileAttr, path, -1, type, buffer, size);
}
int VfsDispatcher::removeFileAttr(const char* path, uint8_t type) {
    if (!checkOwnerOrRoot(path)) return -1;
    VFS_ROUTE_PATH(removeFileAttr, path, -1, type);
}
int VfsDispatcher::getFileMeta(const char* path, file_info_t& out) {
    VFS_ROUTE_PATH(getFileMeta, path, -1, out);
}
int VfsDispatcher::setFilePermissions(const char* path, uint16_t perms) {
    if (!checkOwnerOrRoot(path)) return -1;
    VFS_ROUTE_PATH(setFilePermissions, path, -1, perms);
}
int VfsDispatcher::setFileOwner(const char* path, uint16_t uid, uint16_t gid) {
    if (!checkRoot()) return -1;
    VFS_ROUTE_PATH(setFileOwner, path, -1, uid, gid);
}
int VfsDispatcher::touch(const char* path) {
    if (!checkAccess(path, VFS_ACCESS_W)) return -1;
    VFS_ROUTE_PATH(touch, path, -1);
}

#undef VFS_ROUTE_PATH

int VfsDispatcher::crossCopy(iFileSystemInterface* sb, const char* srel, iFileSystemInterface* db, const char* drel) {
    // Only regular files stream across a mount boundary.
    if (!sb->isFileExist(srel) || sb->isDirectory(srel)) return -1;
    if (db->isFileExist(drel) || db->isDirExist(drel)) return -1;

    // Deliver source content in chunks; the first chunk creates/truncates the
    // destination, the rest append. `size` here is the per-callback chunk cap.
    bool first = true;
    bool ok = true;
    int rc = sb->readFile(srel, 128, [&](char* data, uint32_t n)->bool {
        int w = db->writeFile(drel, data, n, !first);
        first = false;
        if (w < 0) { ok = false; return false; }
        return true;
    });
    if (rc < 0 || !ok) {
        db->deleteFile(drel);
        return -1;
    }
    // Empty source: no chunk was delivered. A zero-byte writeFile does not
    // create the file on every backend (LittleFS opens without O_CREAT for an
    // empty write), so materialise the empty destination with createFile.
    if (first && db->createFile(drel, "", 0) < 0) return -1;
    return 0;
}

int VfsDispatcher::rename(const char* oldPath, const char* newPath) {
    const char *orel = nullptr, *nrel = nullptr;
    iFileSystemInterface* ob = resolve(oldPath, &orel);
    iFileSystemInterface* nb = resolve(newPath, &nrel);
    if (!ob || !nb) return -1;
    if (ob == nb) return ob->rename(orel, nrel);
    // Cross-mount rename == copy to the new backend then drop the source.
    if (crossCopy(ob, orel, nb, nrel) < 0) return -1;
    return ob->deleteFile(orel);
}
int VfsDispatcher::copyFile(const char* sourcePath, const char* destPath) {
    const char *srel = nullptr, *drel = nullptr;
    iFileSystemInterface* sb = resolve(sourcePath, &srel);
    iFileSystemInterface* db = resolve(destPath, &drel);
    if (!sb || !db) return -1;
    if (sb == db) return sb->copyFile(srel, drel);
    return crossCopy(sb, srel, db, drel);
}
int VfsDispatcher::moveFile(const char* oldPath, const char* newPath) {
    const char *orel = nullptr, *nrel = nullptr;
    iFileSystemInterface* ob = resolve(oldPath, &orel);
    iFileSystemInterface* nb = resolve(newPath, &nrel);
    if (!ob || !nb) return -1;
    if (ob == nb) return ob->moveFile(orel, nrel);
    if (crossCopy(ob, orel, nb, nrel) < 0) return -1;
    return ob->deleteFile(orel);
}

uint64_t VfsDispatcher::getTotalSize() {
    iFileSystemInterface* r = rootBackend();
    return r ? r->getTotalSize() : 0;
}
uint64_t VfsDispatcher::getUsedSize() {
    iFileSystemInterface* r = rootBackend();
    return r ? r->getUsedSize() : 0;
}
uint64_t VfsDispatcher::getFreeSize() {
    iFileSystemInterface* r = rootBackend();
    return r ? r->getFreeSize() : 0;
}

pdiutil::string VfsDispatcher::getPWD() const {
    iFileSystemInterface* r = rootBackend();
    return r ? r->getPWD() : s_empty_string;
}
bool VfsDispatcher::setPWD(const char* path) {
    iFileSystemInterface* r = rootBackend();
    return r ? r->setPWD(path) : false;
}
pdiutil::string VfsDispatcher::getLastPWD() const {
    iFileSystemInterface* r = rootBackend();
    return r ? r->getLastPWD() : s_empty_string;
}
void VfsDispatcher::appendFileSeparator(char* path) {
    iFileSystemInterface* r = rootBackend();
    if (r) r->appendFileSeparator(path);
}
void VfsDispatcher::appendFileSeparator(pdiutil::string& path) {
    iFileSystemInterface* r = rootBackend();
    if (r) r->appendFileSeparator(path);
}
bool VfsDispatcher::updatePathNotations(const char* path, pdiutil::string& updatedpath) {
    iFileSystemInterface* r = rootBackend();
    return r ? r->updatePathNotations(path, updatedpath) : false;
}
bool VfsDispatcher::changeDirectory(const char* path) {
    iFileSystemInterface* r = rootBackend();
    return r ? r->changeDirectory(path) : false;
}
const char* VfsDispatcher::getRootDirectory() const {
    iFileSystemInterface* r = rootBackend();
    return r ? r->getRootDirectory() : s_root_path;
}
const char* VfsDispatcher::getHomeDirectory() const {
    iFileSystemInterface* r = rootBackend();
    return r ? r->getHomeDirectory() : s_root_path;
}
const char* VfsDispatcher::getTempDirectory() const {
    iFileSystemInterface* r = rootBackend();
    return r ? r->getTempDirectory() : s_root_path;
}
bool VfsDispatcher::setHomeDirectory(pdiutil::string& homedir) {
    iFileSystemInterface* r = rootBackend();
    return r ? r->setHomeDirectory(homedir) : false;
}
mimetype_t VfsDispatcher::getFileMimeType(const pdiutil::string& path) {
    iFileSystemInterface* r = rootBackend();
    return r ? r->getFileMimeType(path) : MIME_TYPE_APPLICATION_OCTET_STREAM;
}
pdiutil::string VfsDispatcher::basename(const char* path) {
    iFileSystemInterface* r = rootBackend();
    return r ? r->basename(path) : s_empty_string;
}
void VfsDispatcher::applyFileSizeLimit(pdiutil::string& name, uint32_t sizelimit) {
    iFileSystemInterface* r = rootBackend();
    if (r) r->applyFileSizeLimit(name, sizelimit);
}

VfsDispatcher __i_fs;

#endif
