/******************************* FatFs Wrapper ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Bridges the FatFs library with the iFileSystemInterface abstraction, one FAT
volume per instance.

Author          : Suraj I.
created Date    : 21st Oct 2026
******************************************************************************/
#ifndef _EXT_FATFS_WRAPPER_H
#define _EXT_FATFS_WRAPPER_H

#include "interface/pdi/modules/storage/iFileSystemInterface.h"
#include "config/VfsConfig.h"
#include "config/FatFsConfig.h"
#include "interface/pdi/impl/modules/storage/FileLines.h"

#ifdef DEVICE_PROVIDES_FATFS
#include <ff.h>
#else
#include "fatfs/ff.h"
#endif

class FatFsWrapper : public iFileSystemInterface {
public:
    FatFsWrapper(iStorageInterface& storage);

    virtual ~FatFsWrapper();

    /**
     * Mounts the FAT volume the drive number reaches; 0, or a negative error code.
     */
    pdi_err_t mountVolume(uint8_t drive);

    /**
     * Closes every open file and lets go of the volume.
     */
    void unmountVolume();

    /**
     * Makes a FAT volume across the drive number's whole disk as mkfs.vfat does,
     * FAT32 when fat32 is set; 0, or an error code.
     */
    pdi_err_t formatVolume(uint8_t drive, bool fat32);

    bool isMounted() const { return nullptr != m_fs; }

    uint8_t drive() const { return m_drive; }

    int createFile(const char* path, const char* content, int64_t size=-1) override;

    int editFile(const char* path, uint64_t offset, const char* content, uint32_t size) override;

    int writeFile(const char* path, const char* content, uint32_t size, bool append=false) override;

    int readFile(const char* path, uint64_t size, pdiutil::function<bool(char *, uint32_t)> readbackfn, uint64_t offset = 0, const char* readUntilMatchStr=nullptr, bool *didmatchfound=nullptr) override;

    int64_t getOffsetFromLineNumber(const char* path, int linenumber, CallBackVoidArgFn yield = nullptr) override { return FileLines::getOffsetFromLineNumber(*this, path, linenumber, yield); }

    int64_t getLineNumberFromOffset(const char* path, int64_t offset, CallBackVoidArgFn yield = nullptr) override { return FileLines::getLineNumberFromOffset(*this, path, offset, yield); }

    int findInFile(const char* path, const char* findStr, pdiutil::vector<uint32_t> *findindices, int maxindices = -1, int everynthindice = 1, int64_t offset = 0, CallBackVoidArgFn yield = nullptr) override { return FileLines::findInFile(*this, path, findStr, findindices, maxindices, everynthindice, offset, yield); }

    int getLineNumbersInFile(const char* path, pdiutil::vector<uint32_t> &linenumberindices, int maxlinenumbers = -1, int linenumberoffset = 0, CallBackVoidArgFn yield = nullptr) override { return FileLines::getLineNumbersInFile(*this, path, linenumberindices, maxlinenumbers, linenumberoffset, yield); }

    int readLineInFile(const char* path, int32_t linenumber, pdiutil::string &linedata, const char* pattern = nullptr, CallBackVoidArgFn yield = nullptr) override { return FileLines::readLineInFile(*this, path, linenumber, linedata, pattern, yield); }

    pdi_err_t createDirectory(const char* path) override;

    pdi_err_t deleteDirectory(const char* path) override;

    pdi_err_t rename(const char* oldPath, const char* newPath) override;

    pdi_err_t copyFile(const char* sourcePath, const char* destPath, CallBackVoidArgBoolRetFn keepgoing = nullptr) override;

    pdi_err_t moveFile(const char* oldPath, const char* newPath) override;

    pdi_err_t deleteFile(const char* path) override;

    int64_t getFileSize(const char* path) override;

    int getDirFileList(const char* path, pdiutil::vector<file_info_t>& items, const char* pattern = nullptr) override;

    bool isFileExist(const char* path) override;

    bool isDirExist(const char* path) override;

    bool isDirectory(const char* path) override;

    uint64_t getTotalSize() override;

    uint64_t getUsedSize() override;

    uint64_t getFreeSize() override;

    int setFileAttr(const char *path, uint8_t type, const void *buffer, uint32_t size) override;

    int getFileAttr(const char *path, uint8_t type, void *buffer, uint32_t size) override;

    pdi_err_t removeFileAttr(const char *path, uint8_t type) override { return PDI_ERR_NOT_SUPPORTED; }

    pdi_err_t getFileMeta(const char *path, file_info_t &out) override;

    int setFilePermissions(const char *path, uint16_t perms) override;

    int setFileOwner(const char *path, uint16_t uid, uint16_t gid) override { return PDI_ERR_NOT_SUPPORTED; }

    pdi_err_t touch(const char *path) override;

    pdi_fhandle_t openFile(const char* path, uint8_t flags) override;

    int readFileHandle(pdi_fhandle_t handle, char* buffer, uint32_t size) override;

    int writeFileHandle(pdi_fhandle_t handle, const char* content, uint32_t size) override;

    int64_t seekFile(pdi_fhandle_t handle, int64_t offset, file_seek_t whence) override;

    pdi_err_t syncFile(pdi_fhandle_t handle) override;

    pdi_err_t closeFile(pdi_fhandle_t handle) override;

    uint16_t openCount() const override;

protected:
    /**
     * The error the disk last answered with, 0 when it answered none.
     */
    virtual int diskError() const { return 0; }

private:
    /**
     * One open handle, allocated on open and released on close.
     */
    struct fat_open_file_t {
        FIL m_file;
        bool m_append;
    };

    FATFS *m_fs;
    uint8_t m_drive;
    fat_open_file_t *m_openfiles[VFS_MAX_OPEN_FILES];

    /**
     * The path FatFs takes for a mount relative one: the drive and its colon
     * ahead of it.
     */
    pdiutil::string volumePath(const char *path) const;

    /**
     * The entry a path names, with the volume root answered as a directory
     * since FatFs keeps no entry for it.
     */
    FRESULT statPath(const char *path, FILINFO &info, bool &root) const;

    /**
     * Fills the type, size, times and mode an entry shows.
     */
    void fillInfo(const FILINFO &info, file_info_t &out) const;

    /**
     * Stamps the clock's current time as the entry's modification time.
     */
    FRESULT stampNow(const pdiutil::string &fatpath);

    /**
     * A FatFs result in the framework error space, the disk's own error standing in
     * for one FatFs could not name.
     */
    int fatErr(FRESULT fr) const;

    /**
     * A file opened on the heap, since a FatFs file can carry a whole sector of
     * cache.
     */
    FIL *openFat(const pdiutil::string &fatpath, uint8_t mode, int &err) const;

    /**
     * The bytes one cluster of the volume holds.
     */
    uint64_t clusterBytes() const;

    fat_open_file_t *openSlot(pdi_fhandle_t handle);

    /**
     * Removes a directory and everything under it, the path left as it came.
     */
    pdi_err_t removeTree(pdiutil::string &fatpath);
};

#endif // _EXT_FATFS_WRAPPER_H
