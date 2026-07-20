/****************************** VFS Dispatcher ********************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

VfsDispatcher routes iFileSystemInterface calls to a mounted backend selected by
longest-prefix path match. The dispatcher itself owns no storage; each mount
entry points at a concrete iFileSystemInterface (LittleFS root, procfs, sysfs,
tmpfs, etc.). Step A (2026-07-20): read-only mount table, no umount, cross-mount
rename/copy/move returns -1.

Author          : Suraj I.
Created Date    : 20th July 2026
******************************************************************************/

#ifndef _VFS_DISPATCHER_H
#define _VFS_DISPATCHER_H

#include <interface/pdi/modules/storage/iFileSystemInterface.h>
#include <interface/pdi/modules/storage/iStorageInterface.h>
#include <config/VfsConfig.h>

class VfsDispatcher : public iFileSystemInterface {
public:
    VfsDispatcher();
    virtual ~VfsDispatcher() {}

    int8_t mount(const char* prefix, iFileSystemInterface* backend, const char* name, vfs_type_t type);

    uint8_t getMountCount() const { return m_mount_count; }
    const vfs_mount_t* getMount(uint8_t idx) const {
        return (idx < m_mount_count) ? &m_mounts[idx] : nullptr;
    }
    const vfs_mount_t* findMountForPath(const char* path) const;

    int init() override;

    int createFile(const char* path, const char* content, int64_t size = -1) override;
    int editFile(const char* path, uint64_t offset, const char* content, uint32_t size) override;
    int writeFile(const char* path, const char* content, uint32_t size, bool append = false) override;
    int readFile(const char* path, uint64_t size, pdiutil::function<bool(char*, uint32_t)> readbackfn, uint64_t offset = 0, const char* readUntilMatchStr = nullptr, bool* didmatchfound = nullptr) override;

    int64_t getOffsetFromLineNumber(const char* path, int linenumber, CallBackVoidArgFn yield = nullptr) override;
    int64_t getLineNumberFromOffset(const char* path, int64_t offset, CallBackVoidArgFn yield = nullptr) override;
    int findInFile(const char* path, const char* findStr, pdiutil::vector<uint32_t>* findindices, int maxindices = -1, int everynthindice = 1, int64_t offset = 0, CallBackVoidArgFn yield = nullptr) override;
    int getLineNumbersInFile(const char* path, pdiutil::vector<uint32_t>& linenumberindices, int maxlinenumbers = -1, int linenumberoffset = 0, CallBackVoidArgFn yield = nullptr) override;
    int readLineInFile(const char* path, int32_t linenumber, pdiutil::string& linedata, const char* pattern = nullptr, CallBackVoidArgFn yield = nullptr) override;

    int createDirectory(const char* path) override;
    int deleteDirectory(const char* path) override;
    int rename(const char* oldPath, const char* newPath) override;
    int copyFile(const char* sourcePath, const char* destPath) override;
    int moveFile(const char* oldPath, const char* newPath) override;
    int deleteFile(const char* path) override;

    int64_t getFileSize(const char* path) override;
    int getDirFileList(const char* path, pdiutil::vector<file_info_t>& items, const char* pattern = nullptr) override;
    bool isFileExist(const char* path) override;
    bool isDirExist(const char* path) override;
    bool isDirectory(const char* path) override;

    uint64_t getTotalSize() override;
    uint64_t getUsedSize() override;
    uint64_t getFreeSize() override;

    pdiutil::string getPWD() const override;
    bool setPWD(const char* path) override;
    pdiutil::string getLastPWD() const override;

    void appendFileSeparator(char* path) override;
    void appendFileSeparator(pdiutil::string& path) override;
    bool updatePathNotations(const char* path, pdiutil::string& updatedpath) override;
    bool changeDirectory(const char* path) override;
    const char* getRootDirectory() const override;
    const char* getHomeDirectory() const override;
    const char* getTempDirectory() const override;
    bool setHomeDirectory(pdiutil::string& homedir) override;

    mimetype_t getFileMimeType(const pdiutil::string& path) override;
    pdiutil::string basename(const char* path) override;
    void applyFileSizeLimit(pdiutil::string& name, uint32_t sizelimit = FILE_NAME_MAX_SIZE) override;

    int setFileAttr(const char* path, uint8_t type, const void* buffer, uint32_t size) override;
    int getFileAttr(const char* path, uint8_t type, void* buffer, uint32_t size) override;
    int removeFileAttr(const char* path, uint8_t type) override;
    int getFileMeta(const char* path, file_info_t& out) override;
    int setFilePermissions(const char* path, uint16_t perms) override;
    int setFileOwner(const char* path, uint16_t uid, uint16_t gid) override;
    int touch(const char* path) override;

    // POSIX-style access-mode bits accepted by checkAccess.
    static constexpr uint8_t VFS_ACCESS_R = 4;
    static constexpr uint8_t VFS_ACCESS_W = 2;
    static constexpr uint8_t VFS_ACCESS_X = 1;

    // Returns true if the current session has all bits in need_mask on path.
    // Missing files are allowed (creation defer). Root (uid=0) always allowed.
    bool checkAccess(const char* path, uint8_t need_mask);

    // Returns true if the current session is the file's owner OR is root.
    // Missing files are allowed (creation defer).
    bool checkOwnerOrRoot(const char* path);

    // Returns true if the current session is root.
    bool checkRoot();

    // Privileged-scope pair. All three check* helpers bypass while depth > 0.
    // Auth-critical reads (e.g. /etc/shadow during su/login/passwd) enter this
    // scope for the smallest window possible — analogous to setuid on POSIX.
    // Uses depth counter so nested scopes compose correctly.
    void beginPrivileged() { m_priv_depth++; }
    void endPrivileged() { if (m_priv_depth > 0) m_priv_depth--; }
    bool isPrivileged() const { return m_priv_depth > 0; }

protected:
    uint32_t nowEpoch() override { return 0; }

    iFileSystemInterface* resolve(const char* path, const char** relpath_out) const;
    iFileSystemInterface* rootBackend() const;

    vfs_mount_t m_mounts[VFS_MAX_MOUNTS];
    uint8_t m_mount_count;
    uint8_t m_priv_depth;
};

#endif
