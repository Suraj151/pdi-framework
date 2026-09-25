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
    int64_t read(uint64_t, void*, uint64_t) override { return PDI_ERR_NOT_SUPPORTED; }
    int64_t write(uint64_t, const void*, uint64_t) override { return PDI_ERR_NOT_SUPPORTED; }
    bool erase(uint64_t, uint64_t) override { return false; }
    uint64_t size() const override { return 0; }
};

VfsNullStorage s_null_storage;

const char* s_root_path = "/";
const pdiutil::string s_empty_string;

}

VfsDispatcher::VfsDispatcher() : iFileSystemInterface(s_null_storage), m_mount_count(0), m_priv_depth(0)
#ifdef ENABLE_CONTEXTUAL_EXECUTION
    , m_lock(nullptr)
#endif
{}

int8_t VfsDispatcher::mount(const char* prefix, iFileSystemInterface* backend, const char* name, vfs_type_t type) {
    if (!prefix || !backend) {
        return PDI_ERR_INVALID_ARG;
    }
    if (m_mount_count >= VFS_MAX_MOUNTS) {
        return PDI_ERR_NO_SPACE;
    }
    uint32_t plen = strlen(prefix);
    if (plen == 0 || plen > VFS_MOUNT_PREFIX_MAX) {
        return PDI_ERR_INVALID_ARG;
    }
    for (uint8_t i = 0; i < m_mount_count; ++i) {
        if (strcmp(m_mounts[i].m_prefix, prefix) == 0) {
            return PDI_ERR_EXISTS;
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

        if (plen == 1 && m.m_prefix[0] == PATH_SEPARATOR_CHAR) {
            if (best_len == 0) { best_idx = i; best_len = 1; }
            continue;
        }
        if (strncmp(path, m.m_prefix, plen) != 0) continue;
        char after = path[plen];
        if (after != '\0' && after != PATH_SEPARATOR_CHAR) continue;
        if (plen > best_len) { best_idx = i; best_len = plen; }
    }

    if (best_idx == 0xFF) {
        if (relpath_out) *relpath_out = path;
        return nullptr;
    }

    if (relpath_out) {
        if (m_mounts[best_idx].m_prefix_len == 1 && m_mounts[best_idx].m_prefix[0] == PATH_SEPARATOR_CHAR) {
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
        if (m_mounts[i].m_prefix_len == 1 && m_mounts[i].m_prefix[0] == PATH_SEPARATOR_CHAR) {
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
        if (plen == 1 && m.m_prefix[0] == PATH_SEPARATOR_CHAR) {
            if (best_len == 0) { best_idx = i; best_len = 1; }
            continue;
        }
        if (strncmp(path, m.m_prefix, plen) != 0) continue;
        char after = path[plen];
        if (after != '\0' && after != PATH_SEPARATOR_CHAR) continue;
        if (plen > best_len) { best_idx = i; best_len = plen; }
    }
    return (best_idx == 0xFF) ? nullptr : &m_mounts[best_idx];
}

pdi_err_t VfsDispatcher::init() {
    int rc = 0;
#ifdef ENABLE_CONTEXTUAL_EXECUTION
    if (nullptr == m_lock) {
        m_lock = __i_instance.getNewPreemptiveMutexInstance();
    }
#endif
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

#ifdef ENABLE_CONTEXTUAL_EXECUTION
#define VFS_GUARD() iScopedLock _vfs_guard(m_lock)   /* held for this call */
#else
#define VFS_GUARD() ((void)0)
#endif

#define VFS_ROUTE_PATH(_method, _path, _fail, ...)                                  \
    do {                                                                            \
        const char* rel = nullptr;                                                  \
        iFileSystemInterface* b = resolve((_path), &rel);                           \
        if (!b) return (_fail);                                                     \
        return b->_method(rel, ##__VA_ARGS__);                                      \
    } while (0)

int VfsDispatcher::createFile(const char* path, const char* content, int64_t size) {
    VFS_GUARD();
    VFS_ROUTE_PATH(createFile, path, STORAGE_ERROR_NOT_MOUNTED, content, size);
}
int VfsDispatcher::editFile(const char* path, uint64_t offset, const char* content, uint32_t size) {
    VFS_GUARD();
    if (!checkAccess(path, VFS_ACCESS_W)) return PDI_ERR_PERM;
    VFS_ROUTE_PATH(editFile, path, STORAGE_ERROR_NOT_MOUNTED, offset, content, size);
}
int VfsDispatcher::writeFile(const char* path, const char* content, uint32_t size, bool append) {
    VFS_GUARD();
    if (!checkAccess(path, VFS_ACCESS_W)) return PDI_ERR_PERM;
    VFS_ROUTE_PATH(writeFile, path, STORAGE_ERROR_NOT_MOUNTED, content, size, append);
}
int VfsDispatcher::readFile(const char* path, uint64_t size, pdiutil::function<bool(char*, uint32_t)> readbackfn, uint64_t offset, const char* readUntilMatchStr, bool* didmatchfound) {
    VFS_GUARD();
    if (!checkAccess(path, VFS_ACCESS_R)) return PDI_ERR_PERM;
    VFS_ROUTE_PATH(readFile, path, STORAGE_ERROR_NOT_MOUNTED, size, readbackfn, offset, readUntilMatchStr, didmatchfound);
}
int64_t VfsDispatcher::getOffsetFromLineNumber(const char* path, int linenumber, CallBackVoidArgFn yield) {
    VFS_GUARD();
    if (!checkAccess(path, VFS_ACCESS_R)) return PDI_ERR_PERM;
    VFS_ROUTE_PATH(getOffsetFromLineNumber, path, STORAGE_ERROR_NOT_MOUNTED, linenumber, yield);
}
int64_t VfsDispatcher::getLineNumberFromOffset(const char* path, int64_t offset, CallBackVoidArgFn yield) {
    VFS_GUARD();
    if (!checkAccess(path, VFS_ACCESS_R)) return PDI_ERR_PERM;
    VFS_ROUTE_PATH(getLineNumberFromOffset, path, STORAGE_ERROR_NOT_MOUNTED, offset, yield);
}
int VfsDispatcher::findInFile(const char* path, const char* findStr, pdiutil::vector<uint32_t>* findindices, int maxindices, int everynthindice, int64_t offset, CallBackVoidArgFn yield) {
    VFS_GUARD();
    if (!checkAccess(path, VFS_ACCESS_R)) return PDI_ERR_PERM;
    VFS_ROUTE_PATH(findInFile, path, STORAGE_ERROR_NOT_MOUNTED, findStr, findindices, maxindices, everynthindice, offset, yield);
}
int VfsDispatcher::getLineNumbersInFile(const char* path, pdiutil::vector<uint32_t>& linenumberindices, int maxlinenumbers, int linenumberoffset, CallBackVoidArgFn yield) {
    VFS_GUARD();
    if (!checkAccess(path, VFS_ACCESS_R)) return PDI_ERR_PERM;
    VFS_ROUTE_PATH(getLineNumbersInFile, path, STORAGE_ERROR_NOT_MOUNTED, linenumberindices, maxlinenumbers, linenumberoffset, yield);
}
int VfsDispatcher::readLineInFile(const char* path, int32_t linenumber, pdiutil::string& linedata, const char* pattern, CallBackVoidArgFn yield) {
    VFS_GUARD();
    if (!checkAccess(path, VFS_ACCESS_R)) return PDI_ERR_PERM;
    VFS_ROUTE_PATH(readLineInFile, path, STORAGE_ERROR_NOT_MOUNTED, linenumber, linedata, pattern, yield);
}
pdi_err_t VfsDispatcher::createDirectory(const char* path) {
    VFS_GUARD();
    VFS_ROUTE_PATH(createDirectory, path, STORAGE_ERROR_NOT_MOUNTED);
}
pdi_err_t VfsDispatcher::deleteDirectory(const char* path) {
    VFS_GUARD();
    if (!checkAccess(path, VFS_ACCESS_W)) return PDI_ERR_PERM;
    VFS_ROUTE_PATH(deleteDirectory, path, STORAGE_ERROR_NOT_MOUNTED);
}
pdi_err_t VfsDispatcher::deleteFile(const char* path) {
    VFS_GUARD();
    if (!checkAccess(path, VFS_ACCESS_W)) return PDI_ERR_PERM;
    VFS_ROUTE_PATH(deleteFile, path, STORAGE_ERROR_NOT_MOUNTED);
}
int64_t VfsDispatcher::getFileSize(const char* path) {
    VFS_GUARD();
    VFS_ROUTE_PATH(getFileSize, path, STORAGE_ERROR_NOT_MOUNTED);
}
int VfsDispatcher::getDirFileList(const char* path, pdiutil::vector<file_info_t>& items, const char* pattern) {
    VFS_GUARD();
    if (!checkAccess(path, VFS_ACCESS_R)) return PDI_ERR_PERM;
    VFS_ROUTE_PATH(getDirFileList, path, STORAGE_ERROR_NOT_MOUNTED, items, pattern);
}
bool VfsDispatcher::isFileExist(const char* path) {
    VFS_GUARD();
    const char* rel = nullptr;
    iFileSystemInterface* b = resolve(path, &rel);
    return b ? b->isFileExist(rel) : false;
}
bool VfsDispatcher::isDirExist(const char* path) {
    VFS_GUARD();
    const char* rel = nullptr;
    iFileSystemInterface* b = resolve(path, &rel);
    return b ? b->isDirExist(rel) : false;
}
bool VfsDispatcher::isDirectory(const char* path) {
    VFS_GUARD();
    const char* rel = nullptr;
    iFileSystemInterface* b = resolve(path, &rel);
    return b ? b->isDirectory(rel) : false;
}
int VfsDispatcher::setFileAttr(const char* path, uint8_t type, const void* buffer, uint32_t size) {
    VFS_GUARD();
    if (!checkOwnerOrRoot(path)) return PDI_ERR_PERM;
    VFS_ROUTE_PATH(setFileAttr, path, STORAGE_ERROR_NOT_MOUNTED, type, buffer, size);
}
int VfsDispatcher::getFileAttr(const char* path, uint8_t type, void* buffer, uint32_t size) {
    VFS_GUARD();
    VFS_ROUTE_PATH(getFileAttr, path, STORAGE_ERROR_NOT_MOUNTED, type, buffer, size);
}
pdi_err_t VfsDispatcher::removeFileAttr(const char* path, uint8_t type) {
    VFS_GUARD();
    if (!checkOwnerOrRoot(path)) return PDI_ERR_PERM;
    VFS_ROUTE_PATH(removeFileAttr, path, STORAGE_ERROR_NOT_MOUNTED, type);
}
pdi_err_t VfsDispatcher::getFileMeta(const char* path, file_info_t& out) {
    VFS_GUARD();
    VFS_ROUTE_PATH(getFileMeta, path, STORAGE_ERROR_NOT_MOUNTED, out);
}
int VfsDispatcher::setFilePermissions(const char* path, uint16_t perms) {
    VFS_GUARD();
    if (!checkOwnerOrRoot(path)) return PDI_ERR_PERM;
    VFS_ROUTE_PATH(setFilePermissions, path, STORAGE_ERROR_NOT_MOUNTED, perms);
}
int VfsDispatcher::setFileOwner(const char* path, uint16_t uid, uint16_t gid) {
    VFS_GUARD();
    if (!checkRoot()) return PDI_ERR_PERM;
    VFS_ROUTE_PATH(setFileOwner, path, STORAGE_ERROR_NOT_MOUNTED, uid, gid);
}
pdi_err_t VfsDispatcher::touch(const char* path) {
    VFS_GUARD();
    if (!checkAccess(path, VFS_ACCESS_W)) return PDI_ERR_PERM;
    VFS_ROUTE_PATH(touch, path, STORAGE_ERROR_NOT_MOUNTED);
}

#undef VFS_ROUTE_PATH

iFileSystemInterface* VfsDispatcher::resolveHandle(pdi_fhandle_t handle, pdi_fhandle_t& backend_handle) const {

    if (handle < 0) {
        return nullptr;
    }

    uint8_t mount_id = (uint8_t)((handle >> VFS_HANDLE_MOUNT_SHIFT) & VFS_HANDLE_MOUNT_MAX);
    if (0 == mount_id || mount_id > m_mount_count) {
        return nullptr;
    }

    backend_handle = handle & VFS_HANDLE_BACKEND_MASK;
    return m_mounts[mount_id - 1].m_backend;
}

/**
 * @brief Opens a file on whichever mount owns the path. The backend keeps the
 *        open file; the handle handed back only records which mount it came
 *        from, so nothing is tracked here.
 * @param path The path of the file to open.
 * @param flags Combination of file_open_flag_t values.
 * @return A handle of 0 or above, or a negative error code on failure. A mount
 *         whose backend has no handles answers PDI_ERR_NOT_SUPPORTED, and the
 *         caller falls back to the path based calls.
 */
pdi_fhandle_t VfsDispatcher::openFile(const char* path, uint8_t flags) {

    if (!path || '\0' == path[0]) {
        return (pdi_fhandle_t)STORAGE_ERROR_BAD_PATH;
    }

    VFS_GUARD();

    uint8_t need = 0;
    if (flags & FILE_OPEN_READ) need |= VFS_ACCESS_R;
    if (flags & (FILE_OPEN_WRITE | FILE_OPEN_CREATE | FILE_OPEN_TRUNCATE | FILE_OPEN_APPEND)) need |= VFS_ACCESS_W;
    if (need && !checkAccess(path, need)) {
        return (pdi_fhandle_t)PDI_ERR_PERM;
    }

    const char* rel = nullptr;
    iFileSystemInterface* backend = resolve(path, &rel);
    if (!backend) {
        return (pdi_fhandle_t)STORAGE_ERROR_NOT_MOUNTED;
    }

    uint8_t mount_id = 0;
    for (uint8_t i = 0; i < m_mount_count; ++i) {
        if (m_mounts[i].m_backend == backend) {
            mount_id = (uint8_t)(i + 1);
            break;
        }
    }

    if (0 == mount_id) {
        return (pdi_fhandle_t)STORAGE_ERROR_NOT_MOUNTED;
    }

    pdi_fhandle_t handle = backend->openFile(rel, flags);
    if (handle < 0) {
        return handle;
    }

    if (handle > VFS_HANDLE_BACKEND_MASK) {
        backend->closeFile(handle);
        return (pdi_fhandle_t)PDI_ERR_RANGE;
    }

    return (pdi_fhandle_t)(((pdi_fhandle_t)mount_id << VFS_HANDLE_MOUNT_SHIFT) | handle);
}

/**
 * @brief Reads from an open handle, advancing its position by what it read.
 * @param handle Handle returned by openFile.
 * @param buffer Destination for the bytes read.
 * @param size Capacity of the buffer in bytes.
 * @return The number of bytes read, 0 at end of file, or a negative error code.
 */
int VfsDispatcher::readFileHandle(pdi_fhandle_t handle, char* buffer, uint32_t size) {

    VFS_GUARD();
    pdi_fhandle_t bh = 0;
    iFileSystemInterface* backend = resolveHandle(handle, bh);
    if (!backend) {
        return PDI_ERR_INVALID_ARG;
    }

    return backend->readFileHandle(bh, buffer, size);
}

/**
 * @brief Writes to an open handle, advancing its position by what it wrote.
 * @param handle Handle returned by openFile.
 * @param content The bytes to write.
 * @param size The number of bytes to write.
 * @return The number of bytes written, or a negative error code on failure.
 */
int VfsDispatcher::writeFileHandle(pdi_fhandle_t handle, const char* content, uint32_t size) {

    VFS_GUARD();
    pdi_fhandle_t bh = 0;
    iFileSystemInterface* backend = resolveHandle(handle, bh);
    if (!backend) {
        return PDI_ERR_INVALID_ARG;
    }

    return backend->writeFileHandle(bh, content, size);
}

/**
 * @brief Moves the position of an open handle.
 * @param handle Handle returned by openFile.
 * @param offset Offset to move by, relative to whence.
 * @param whence Reference point for the offset.
 * @return The new position, or a negative error code on failure.
 */
int64_t VfsDispatcher::seekFile(pdi_fhandle_t handle, int64_t offset, file_seek_t whence) {

    VFS_GUARD();
    pdi_fhandle_t bh = 0;
    iFileSystemInterface* backend = resolveHandle(handle, bh);
    if (!backend) {
        return PDI_ERR_INVALID_ARG;
    }

    return backend->seekFile(bh, offset, whence);
}

/**
 * @brief Pushes anything an open handle still holds out to storage, leaving the
 *        handle open.
 * @param handle Handle returned by openFile.
 * @return 0 on success, or a negative error code on failure.
 */
pdi_err_t VfsDispatcher::syncFile(pdi_fhandle_t handle) {

    VFS_GUARD();
    pdi_fhandle_t bh = 0;
    iFileSystemInterface* backend = resolveHandle(handle, bh);
    if (!backend) {
        return PDI_ERR_INVALID_ARG;
    }

    return backend->syncFile(bh);
}

/**
 * @brief Closes an open handle, committing anything the backend still holds.
 * @param handle Handle returned by openFile.
 * @return 0 on success, or a negative error code on failure.
 */
pdi_err_t VfsDispatcher::closeFile(pdi_fhandle_t handle) {

    VFS_GUARD();
    pdi_fhandle_t bh = 0;
    iFileSystemInterface* backend = resolveHandle(handle, bh);
    if (!backend) {
        return PDI_ERR_INVALID_ARG;
    }

    return backend->closeFile(bh);
}

int VfsDispatcher::crossCopy(iFileSystemInterface* sb, const char* srel, iFileSystemInterface* db, const char* drel) {
    // Only regular files stream across a mount boundary.
    if (!sb->isFileExist(srel) || sb->isDirectory(srel)) return STORAGE_ERROR_NOT_A_FILE;
    if (db->isFileExist(drel) || db->isDirExist(drel)) return PDI_ERR_EXISTS;

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
        return STORAGE_ERROR_BACKEND;
    }
    // Empty source: no chunk was delivered. A zero-byte writeFile does not
    // create the file on every backend (LittleFS opens without O_CREAT for an
    // empty write), so materialise the empty destination with createFile.
    if (first && db->createFile(drel, "", 0) < 0) return STORAGE_ERROR_BACKEND;
    return 0;
}

pdi_err_t VfsDispatcher::rename(const char* oldPath, const char* newPath) {
    VFS_GUARD();
    if (!checkAccess(newPath, VFS_ACCESS_W)) return PDI_ERR_PERM;
    const char *orel = nullptr, *nrel = nullptr;
    iFileSystemInterface* ob = resolve(oldPath, &orel);
    iFileSystemInterface* nb = resolve(newPath, &nrel);
    if (!ob || !nb) return STORAGE_ERROR_NOT_MOUNTED;
    if (ob == nb) return ob->rename(orel, nrel);
    // Cross-mount rename == copy to the new backend then drop the source.
    int rc = crossCopy(ob, orel, nb, nrel);
    if (rc < 0) return rc;
    return ob->deleteFile(orel);
}
pdi_err_t VfsDispatcher::copyFile(const char* sourcePath, const char* destPath) {
    VFS_GUARD();
    if (!checkAccess(sourcePath, VFS_ACCESS_R)) return PDI_ERR_PERM;
    if (!checkAccess(destPath, VFS_ACCESS_W)) return PDI_ERR_PERM;
    const char *srel = nullptr, *drel = nullptr;
    iFileSystemInterface* sb = resolve(sourcePath, &srel);
    iFileSystemInterface* db = resolve(destPath, &drel);
    if (!sb || !db) return STORAGE_ERROR_NOT_MOUNTED;
    if (sb == db) return sb->copyFile(srel, drel);
    return crossCopy(sb, srel, db, drel);
}
pdi_err_t VfsDispatcher::moveFile(const char* oldPath, const char* newPath) {
    VFS_GUARD();
    if (!checkAccess(newPath, VFS_ACCESS_W)) return PDI_ERR_PERM;
    const char *orel = nullptr, *nrel = nullptr;
    iFileSystemInterface* ob = resolve(oldPath, &orel);
    iFileSystemInterface* nb = resolve(newPath, &nrel);
    if (!ob || !nb) return STORAGE_ERROR_NOT_MOUNTED;
    if (ob == nb) return ob->moveFile(orel, nrel);
    int rc = crossCopy(ob, orel, nb, nrel);
    if (rc < 0) return rc;
    return ob->deleteFile(orel);
}

uint64_t VfsDispatcher::getTotalSize() {
    VFS_GUARD();
    iFileSystemInterface* r = rootBackend();
    return r ? r->getTotalSize() : 0;
}
uint64_t VfsDispatcher::getUsedSize() {
    VFS_GUARD();
    iFileSystemInterface* r = rootBackend();
    return r ? r->getUsedSize() : 0;
}
uint64_t VfsDispatcher::getFreeSize() {
    VFS_GUARD();
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
    VFS_GUARD();
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
