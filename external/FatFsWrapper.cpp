/******************************* FatFs Wrapper ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Bridges the FatFs library with the iFileSystemInterface abstraction, one FAT
volume per instance.

Author          : Suraj I.
created Date    : 21st Oct 2026
******************************************************************************/

#include "FatFsWrapper.h"

namespace {

/* the FatFs date of 1980-01-01 00:00, which an entry written without a clock carries */
const WORD s_fat_date_unset = 0x0021;

/* FAT keeps the local wall time, this far ahead of UTC */
const int32_t s_fat_local_offset = (int32_t)(TZ_SEC + DST_SEC);

/* a FatFs result in the framework error space, the exact result kept in the FATFS band */
int fatResult(FRESULT fr) {
    return (FR_OK == fr) ? 0 : PDI_ERR_FROM_FAT(fr);
}

/* seconds since the epoch a FatFs local date and time name, 0 when the entry carries none */
uint32_t fatTimeToEpoch(WORD fdate, WORD ftime) {
    if (s_fat_date_unset == fdate && 0 == ftime) {
        return 0;
    }
    datetime_t dt;
    dt.m_year = (uint16_t)(1980 + (fdate >> 9));
    dt.m_month = (uint8_t)((fdate >> 5) & 0x0F);
    dt.m_day = (uint8_t)(fdate & 0x1F);
    dt.m_hour = (uint8_t)(ftime >> 11);
    dt.m_minute = (uint8_t)((ftime >> 5) & 0x3F);
    dt.m_second = (uint8_t)((ftime & 0x1F) * 2);
    return (uint32_t)((int64_t)DateTimeToEpoch(dt) - s_fat_local_offset);
}

/* the packed FatFs local date and time for an epoch, 1980-01-01 for one before it */
DWORD epochToFatTime(uint32_t epoch) {
    datetime_t dt;
    EpochToDateTime((uint32_t)((int64_t)epoch + s_fat_local_offset), dt);
    if (dt.m_year < 1980) {
        return (DWORD)s_fat_date_unset << 16;
    }
    return ((DWORD)(dt.m_year - 1980) << 25) | ((DWORD)dt.m_month << 21) | ((DWORD)dt.m_day << 16) |
           ((DWORD)dt.m_hour << 11) | ((DWORD)dt.m_minute << 5) | ((DWORD)dt.m_second >> 1);
}

/* closes a file openFat gave and frees it */
FRESULT closeFat(FIL *&file) {
    FRESULT fr = f_close(file);
    pdiutil::safe_delete(file);
    return fr;
}

}

/**
 * A FatFs result in the framework error space, the disk's own error standing in for
 * one FatFs could not name.
 */
int FatFsWrapper::fatErr(FRESULT fr) const {
    int disk = diskError();
    return ((FR_DISK_ERR == fr || FR_NOT_READY == fr) && disk < 0) ? disk : fatResult(fr);
}

/**
 * A file opened on the heap, since a FatFs file can carry a whole sector of
 * cache.
 */
FIL *FatFsWrapper::openFat(const pdiutil::string &fatpath, uint8_t mode, int &err) const {
    FIL *file = pdiutil::safe_new<FIL>();
    if (nullptr == file) {
        err = PDI_ERR_NO_MEM;
        return nullptr;
    }
    err = fatErr(f_open(file, fatpath.c_str(), (BYTE)mode));
    if (0 != err) {
        pdiutil::safe_delete(file);
    }
    return file;
}

FatFsWrapper::FatFsWrapper(iStorageInterface& storage)
    : iFileSystemInterface(storage), m_fs(nullptr), m_drive(0) {
    for (uint8_t i = 0; i < VFS_MAX_OPEN_FILES; i++) {
        m_openfiles[i] = nullptr;
    }
}

FatFsWrapper::~FatFsWrapper() {
    unmountVolume();
}

/**
 * Mounts the FAT volume the drive number reaches; 0, or a negative error code.
 */
pdi_err_t FatFsWrapper::mountVolume(uint8_t drive) {
    if (nullptr != m_fs) {
        return PDI_ERR_BUSY;
    }
    m_fs = pdiutil::safe_new<FATFS>();
    if (nullptr == m_fs) {
        return PDI_ERR_NO_MEM;
    }
    m_drive = drive;
    FRESULT fr = f_mount(m_fs, volumePath(nullptr).c_str(), 1);
    if (FR_OK != fr) {
        f_mount(nullptr, volumePath(nullptr).c_str(), 0);
        pdiutil::safe_delete(m_fs);
    }
    return (pdi_err_t)fatErr(fr);
}

/**
 * Makes a FAT volume across the drive number's whole disk as mkfs.vfat does, FAT32
 * when fat32 is set; 0, or an error code.
 */
pdi_err_t FatFsWrapper::formatVolume(uint8_t drive, bool fat32) {
    if (nullptr != m_fs) {
        return PDI_ERR_BUSY;
    }
    uint8_t *work = pdiutil::safe_new_array<uint8_t>(FF_MAX_SS);
    if (nullptr == work) {
        return PDI_ERR_NO_MEM;
    }
    m_drive = drive;
    MKFS_PARM opt;
    memset(&opt, 0, sizeof(opt));
    opt.fmt = (fat32 ? FM_FAT32 : FM_ANY) | FM_SFD;
    opt.n_fat = 2;
    FRESULT fr = f_mkfs(volumePath(nullptr).c_str(), &opt, work, FF_MAX_SS);
    pdiutil::safe_delete_array(work);
    return (pdi_err_t)fatErr(fr);
}

/**
 * Closes every open file and lets go of the volume.
 */
void FatFsWrapper::unmountVolume() {
    for (uint8_t i = 0; i < VFS_MAX_OPEN_FILES; i++) {
        if (nullptr != m_openfiles[i]) {
            f_close(&m_openfiles[i]->m_file);
            pdiutil::safe_delete(m_openfiles[i]);
        }
    }
    if (nullptr != m_fs) {
        f_mount(nullptr, volumePath(nullptr).c_str(), 0);
        pdiutil::safe_delete(m_fs);
    }
}

/**
 * The path FatFs takes for a mount relative one: the drive and its colon
 * ahead of it.
 */
pdiutil::string FatFsWrapper::volumePath(const char *path) const {
    pdiutil::string out;
    out += (char)('0' + m_drive);
    out += ':';
    if (nullptr == path || PATH_SEPARATOR_CHAR != path[0]) {
        out += PATH_SEPARATOR_CHAR;
    }
    if (nullptr != path) {
        out += path;
    }
    while (out.length() > FAT_DRIVE_PREFIX_SIZE + 1 && PATH_SEPARATOR_CHAR == out[out.length() - 1]) {
        out.erase(out.length() - 1, 1);
    }
    return out;
}

/**
 * The entry a path names, with the volume root answered as a directory
 * since FatFs keeps no entry for it.
 */
FRESULT FatFsWrapper::statPath(const char *path, FILINFO &info, bool &root) const {
    memset(&info, 0, sizeof(info));
    pdiutil::string fatpath = volumePath(path);
    root = (fatpath.length() == FAT_DRIVE_PREFIX_SIZE + 1);
    if (root) {
        info.fattrib = AM_DIR;
        return FR_OK;
    }
    return f_stat(fatpath.c_str(), &info);
}

/**
 * Fills the type, size, times and mode an entry shows.
 */
void FatFsWrapper::fillInfo(const FILINFO &info, file_info_t &out) const {
    bool dir = (0 != (info.fattrib & AM_DIR));
    out.m_type = dir ? FILE_TYPE_DIR : FILE_TYPE_REG;
    out.m_size = dir ? 0 : (uint64_t)info.fsize;
    out.m_mtime = fatTimeToEpoch(info.fdate, info.ftime);
    out.m_ctime = out.m_mtime;
    out.m_perms = (uint16_t)FAT_PERM_DEFAULT;
    if (0 != (info.fattrib & AM_RDO)) {
        out.m_perms &= (uint16_t)~FAT_PERM_WRITE_BITS;
    }
    out.m_uid = 0;
    out.m_gid = 0;
}

/**
 * Stamps the clock's current time as the entry's modification time.
 */
FRESULT FatFsWrapper::stampNow(const pdiutil::string &fatpath) {
    uint32_t now = nowEpoch();
    if (0 == now) {
        return FR_OK;
    }
    DWORD packed = epochToFatTime(now);
    FILINFO info;
    memset(&info, 0, sizeof(info));
    info.fdate = (WORD)(packed >> 16);
    info.ftime = (WORD)(packed & 0xFFFF);
    return f_utime(fatpath.c_str(), &info);
}

/**
 * Creates a file holding the content, refusing one that exists; the bytes written,
 * or a negative error code.
 */
int FatFsWrapper::createFile(const char* path, const char* content, int64_t size) {
    int err = 0;
    FIL *file = openFat(volumePath(path), FA_WRITE | FA_CREATE_NEW, err);
    if (nullptr == file) {
        return err;
    }
    uint32_t len = (size < 0) ? ((nullptr != content) ? (uint32_t)strlen(content) : 0) : (uint32_t)size;
    UINT written = 0;
    FRESULT fr = (len > 0 && nullptr != content) ? f_write(file, content, len, &written) : FR_OK;
    FRESULT closed = closeFat(file);
    if (FR_OK != fr || FR_OK != closed) {
        return fatErr((FR_OK != fr) ? fr : closed);
    }
    return (written < len) ? (int)PDI_ERR_NO_SPACE : (int)written;
}

/**
 * Overwrites part of a file from the offset, zero filling a gap past its end; the
 * bytes written, or a negative error code.
 */
int FatFsWrapper::editFile(const char* path, uint64_t offset, const char* content, uint32_t size) {
    if (0 == size) {
        return 0;
    }
    if (nullptr == content) {
        return PDI_ERR_NULL_PTR;
    }
    int err = 0;
    FIL *file = openFat(volumePath(path), FA_WRITE, err);
    if (nullptr == file) {
        return err;
    }

    FRESULT fr = FR_OK;
    UINT written = 0;
    uint64_t end = f_size(file);
    if (offset > end) {
        uint8_t pad[64];
        memset(pad, 0, sizeof(pad));
        fr = f_lseek(file, (FSIZE_t)end);
        while (FR_OK == fr && end < offset) {
            UINT chunk = (offset - end > sizeof(pad)) ? (UINT)sizeof(pad) : (UINT)(offset - end);
            fr = f_write(file, pad, chunk, &written);
            if (FR_OK == fr && written < chunk) {
                closeFat(file);
                return PDI_ERR_NO_SPACE;
            }
            end += written;
        }
    }

    written = 0;
    if (FR_OK == fr) {
        fr = f_lseek(file, (FSIZE_t)offset);
    }
    if (FR_OK == fr) {
        fr = f_write(file, content, size, &written);
    }
    FRESULT closed = closeFat(file);
    if (FR_OK != fr || FR_OK != closed) {
        return fatErr((FR_OK != fr) ? fr : closed);
    }
    return (written < size) ? (int)PDI_ERR_NO_SPACE : (int)written;
}

/**
 * Writes a file whole, or appends to it, creating it when missing; the bytes
 * written, or a negative error code.
 */
int FatFsWrapper::writeFile(const char *path, const char *content, uint32_t size, bool append) {
    if (0 == size) {
        return 0;
    }
    if (nullptr == content) {
        return PDI_ERR_NULL_PTR;
    }
    int err = 0;
    FIL *file = openFat(volumePath(path), FA_WRITE | (append ? FA_OPEN_APPEND : FA_CREATE_ALWAYS), err);
    if (nullptr == file) {
        return err;
    }
    UINT written = 0;
    FRESULT fr = f_write(file, content, size, &written);
    FRESULT closed = closeFat(file);
    if (FR_OK != fr || FR_OK != closed) {
        return fatErr((FR_OK != fr) ? fr : closed);
    }
    return (written < size) ? (int)PDI_ERR_NO_SPACE : (int)written;
}

/**
 * Delivers a file from the offset in pieces of at most size bytes, stopping ahead
 * of match when given; the bytes delivered.
 */
int FatFsWrapper::readFile(const char* path, uint64_t size, pdiutil::function<bool(char *, uint32_t)> readbackfn, uint64_t offset, const char* readUntilMatchStr, bool *didmatchfound) {
    if (nullptr == readbackfn || 0 == size) {
        return PDI_ERR_INVALID_ARG;
    }
    int err = 0;
    FIL *file = openFat(volumePath(path), FA_READ, err);
    if (nullptr == file) {
        return err;
    }

    uint64_t fsize = f_size(file);
    if (offset >= fsize) {
        closeFat(file);
        return 0;
    }
    uint64_t remaining = fsize - offset;
    uint32_t matchlen = (nullptr != readUntilMatchStr) ? (uint32_t)strlen(readUntilMatchStr) : 0;
    uint32_t chunk = (size < remaining) ? (uint32_t)size : (uint32_t)remaining;
    char *buffer = pdiutil::safe_new_array<char>(chunk + matchlen);
    FRESULT fr = (nullptr == buffer) ? FR_OK : f_lseek(file, (FSIZE_t)offset);

    uint32_t filled = 0;
    uint32_t delivered = 0;
    bool stop = (nullptr == buffer || FR_OK != fr);
    while (!stop) {
        UINT got = 0;
        uint32_t want = (chunk < remaining) ? chunk : (uint32_t)remaining;
        if (want > 0) {
            fr = f_read(file, buffer + filled, want, &got);
            if (FR_OK != fr) {
                break;
            }
            remaining = (0 == got) ? 0 : remaining - got;
            filled += got;
        }
        bool eof = (0 == remaining);

        uint32_t ready = filled;
        if (matchlen > 0) {
            int32_t at = __strstr(buffer, filled, readUntilMatchStr, matchlen, 0);
            if (at >= 0) {
                ready = (uint32_t)at;
                stop = true;
                if (nullptr != didmatchfound) {
                    *didmatchfound = true;
                }
            } else if (!eof) {
                ready = (filled > matchlen - 1) ? filled - (matchlen - 1) : 0;
            }
        }

        uint32_t sent = 0;
        while (sent < ready) {
            uint32_t piece = (ready - sent > size) ? (uint32_t)size : ready - sent;
            bool more = readbackfn(buffer + sent, piece);
            sent += piece;
            if (!more) {
                stop = true;
                break;
            }
        }
        delivered += sent;
        if (stop || eof) {
            break;
        }
        memmove(buffer, buffer + ready, filled - ready);
        filled -= ready;
    }

    bool nomem = (nullptr == buffer);
    pdiutil::safe_delete_array(buffer);
    closeFat(file);
    if (nomem) {
        return PDI_ERR_NO_MEM;
    }
    return (FR_OK != fr) ? fatErr(fr) : (int)delivered;
}

/**
 * Creates a directory along with any missing parent; 0, or a negative error code.
 */
pdi_err_t FatFsWrapper::createDirectory(const char* path) {
    if (nullptr == path || '\0' == path[0]) {
        return PDI_ERR_INVALID_ARG;
    }
    pdiutil::string fatpath = volumePath(path);
    uint32_t len = (uint32_t)fatpath.length();
    for (uint32_t i = FAT_DRIVE_PREFIX_SIZE + 1; i <= len; i++) {
        if (i < len && PATH_SEPARATOR_CHAR != fatpath[i]) {
            continue;
        }
        pdiutil::string step = fatpath.substr(0, i);
        FRESULT fr = f_mkdir(step.c_str());
        if (FR_OK != fr && FR_EXIST != fr) {
            return (pdi_err_t)fatErr(fr);
        }
    }
    return PDI_OK;
}

pdi_err_t FatFsWrapper::removeTree(pdiutil::string &fatpath) {
    FF_DIR dir;
    FILINFO info;
    FRESULT fr = f_opendir(&dir, fatpath.c_str());
    uint32_t base = (uint32_t)fatpath.length();
    while (FR_OK == fr) {
        fr = f_readdir(&dir, &info);
        if (FR_OK != fr || '\0' == info.fname[0]) {
            break;
        }
        fatpath += PATH_SEPARATOR_CHAR;
        fatpath += info.fname;
        if (0 != (info.fattrib & AM_DIR)) {
            pdi_err_t inner = removeTree(fatpath);
            if (PDI_OK != inner) {
                f_closedir(&dir);
                fatpath.erase(base, fatpath.length() - base);
                return inner;
            }
        } else {
            fr = f_unlink(fatpath.c_str());
        }
        fatpath.erase(base, fatpath.length() - base);
    }
    f_closedir(&dir);
    if (FR_OK == fr) {
        fr = f_unlink(fatpath.c_str());
    }
    return (pdi_err_t)fatErr(fr);
}

/**
 * Deletes a directory and everything under it; 0, or a negative error code.
 */
pdi_err_t FatFsWrapper::deleteDirectory(const char* path) {
    FILINFO info;
    bool root = false;
    FRESULT fr = statPath(path, info, root);
    if (root) {
        return STORAGE_ERROR_BAD_PATH;
    }
    if (FR_OK != fr) {
        return (pdi_err_t)fatErr(fr);
    }
    if (0 == (info.fattrib & AM_DIR)) {
        return STORAGE_ERROR_NOT_A_DIRECTORY;
    }
    pdiutil::string fatpath = volumePath(path);
    return removeTree(fatpath);
}

/**
 * Renames a file or directory, replacing a file at the new path; 0, or a negative
 * error code.
 */
pdi_err_t FatFsWrapper::rename(const char* oldPath, const char* newPath) {
    pdiutil::string from = volumePath(oldPath);
    pdiutil::string to = volumePath(newPath);
    FILINFO info;
    FRESULT fr = f_stat(to.c_str(), &info);
    if (FR_OK == fr && 0 == (info.fattrib & AM_DIR)) {
        fr = f_unlink(to.c_str());
        if (FR_OK != fr) {
            return (pdi_err_t)fatErr(fr);
        }
    }
    return (pdi_err_t)fatErr(f_rename(from.c_str(), to.c_str()));
}

/**
 * Copies a file, removing a partial copy on failure or when keepgoing says stop
 * (PDI_ERR_ABORTED); 0, or an error code.
 */
pdi_err_t FatFsWrapper::copyFile(const char* sourcePath, const char* destPath, CallBackVoidArgBoolRetFn keepgoing) {
    int err = 0;
    FIL *source = openFat(volumePath(sourcePath), FA_READ, err);
    if (nullptr == source) {
        return (pdi_err_t)err;
    }
    pdiutil::string to = volumePath(destPath);
    FIL *dest = openFat(to, FA_WRITE | FA_CREATE_ALWAYS, err);
    char *buffer = (nullptr == dest) ? nullptr : pdiutil::safe_new_array<char>(FF_MIN_SS);
    if (nullptr == dest || nullptr == buffer) {
        closeFat(source);
        if (nullptr != dest) {
            closeFat(dest);
            f_unlink(to.c_str());
        }
        return (pdi_err_t)((nullptr == dest) ? err : PDI_ERR_NO_MEM);
    }

    FRESULT fr = FR_OK;
    UINT got = 0;
    UINT put = 0;
    do {
        fr = f_read(source, buffer, FF_MIN_SS, &got);
        if (FR_OK == fr && got > 0) {
            fr = f_write(dest, buffer, got, &put);
            err = (FR_OK == fr && put < got) ? (int)PDI_ERR_NO_SPACE : 0;
        }
        if (FR_OK == fr && 0 == err && got > 0 && nullptr != keepgoing && !keepgoing()) {
            err = PDI_ERR_ABORTED;
        }
    } while (FR_OK == fr && 0 == err && got > 0);

    pdiutil::safe_delete_array(buffer);
    closeFat(source);
    FRESULT closed = closeFat(dest);
    if (FR_OK == fr && 0 == err && FR_OK != closed) {
        fr = closed;
    }
    if (FR_OK != fr || 0 != err) {
        f_unlink(to.c_str());
        return (pdi_err_t)((0 != err) ? err : fatErr(fr));
    }
    return PDI_OK;
}

/**
 * Moves a file to a new path; 0, or a negative error code.
 */
pdi_err_t FatFsWrapper::moveFile(const char* oldPath, const char* newPath) {
    return rename(oldPath, newPath);
}

/**
 * Deletes a file, refusing a directory; 0, or a negative error code.
 */
pdi_err_t FatFsWrapper::deleteFile(const char* path) {
    FILINFO info;
    bool root = false;
    FRESULT fr = statPath(path, info, root);
    if (FR_OK != fr) {
        return (pdi_err_t)fatErr(fr);
    }
    if (0 != (info.fattrib & AM_DIR)) {
        return STORAGE_ERROR_NOT_A_FILE;
    }
    return (pdi_err_t)fatErr(f_unlink(volumePath(path).c_str()));
}

/**
 * The size of a file in bytes, or a negative error code.
 */
int64_t FatFsWrapper::getFileSize(const char* path) {
    FILINFO info;
    bool root = false;
    FRESULT fr = statPath(path, info, root);
    if (FR_OK != fr) {
        return fatErr(fr);
    }
    if (0 != (info.fattrib & AM_DIR)) {
        return STORAGE_ERROR_NOT_A_FILE;
    }
    return (int64_t)info.fsize;
}

/**
 * Lists a directory, keeping names that start with the pattern; every m_name is a
 * heap copy the caller deletes.
 */
int FatFsWrapper::getDirFileList(const char* path, pdiutil::vector<file_info_t>& items, const char* pattern) {
    FF_DIR dir;
    FILINFO info;
    FRESULT fr = f_opendir(&dir, volumePath(path).c_str());
    uint32_t patternlen = (nullptr != pattern) ? (uint32_t)strlen(pattern) : 0;
    while (FR_OK == fr) {
        fr = f_readdir(&dir, &info);
        if (FR_OK != fr || '\0' == info.fname[0]) {
            break;
        }
        uint32_t namelen = (uint32_t)strlen(info.fname);
        if (patternlen > 0 && (patternlen > namelen || 0 != strncmp(info.fname, pattern, patternlen))) {
            continue;
        }
        file_info_t entry;
        memset(&entry, 0, sizeof(entry));
        entry.m_name = pdiutil::safe_new_array<char>(namelen + 1);
        if (nullptr == entry.m_name) {
            continue;
        }
        memcpy(entry.m_name, info.fname, namelen + 1);
        fillInfo(info, entry);
        items.push_back(entry);
    }
    f_closedir(&dir);
    return fatErr(fr);
}

bool FatFsWrapper::isFileExist(const char* path) {
    FILINFO info;
    bool root = false;
    return FR_OK == statPath(path, info, root) && 0 == (info.fattrib & AM_DIR);
}

bool FatFsWrapper::isDirExist(const char* path) {
    FILINFO info;
    bool root = false;
    return FR_OK == statPath(path, info, root) && 0 != (info.fattrib & AM_DIR);
}

bool FatFsWrapper::isDirectory(const char* path) {
    return isDirExist(path);
}

/**
 * The bytes one cluster of the volume holds.
 */
uint64_t FatFsWrapper::clusterBytes() const {
#if FF_MAX_SS != FF_MIN_SS
    return (uint64_t)m_fs->csize * m_fs->ssize;
#else
    return (uint64_t)m_fs->csize * FF_MIN_SS;
#endif
}

/**
 * The bytes the volume's data clusters hold.
 */
uint64_t FatFsWrapper::getTotalSize() {
    if (nullptr == m_fs) {
        return 0;
    }
    return (uint64_t)(m_fs->n_fatent - 2) * clusterBytes();
}

/**
 * The bytes the volume's free clusters hold.
 */
uint64_t FatFsWrapper::getFreeSize() {
    FATFS *fs = nullptr;
    DWORD clusters = 0;
    if (nullptr == m_fs || FR_OK != f_getfree(volumePath(nullptr).c_str(), &clusters, &fs)) {
        return 0;
    }
    return (uint64_t)clusters * clusterBytes();
}

uint64_t FatFsWrapper::getUsedSize() {
    uint64_t total = getTotalSize();
    uint64_t freebytes = getFreeSize();
    return (total > freebytes) ? total - freebytes : 0;
}

/**
 * Sets the modification time and, through the read-only flag, the write bits; the
 * size taken, or a negative error code.
 */
int FatFsWrapper::setFileAttr(const char *path, uint8_t type, const void *buffer, uint32_t size) {
    if (nullptr == buffer || 0 == size) {
        return PDI_ERR_INVALID_ARG;
    }
    if (FILE_ATTR_PERMS == type && size >= sizeof(uint16_t)) {
        int done = setFilePermissions(path, *(const uint16_t *)buffer);
        return (done < 0) ? done : (int)size;
    }
    if (FILE_ATTR_MTIME == type && size >= sizeof(uint32_t)) {
        DWORD packed = epochToFatTime(*(const uint32_t *)buffer);
        FILINFO info;
        memset(&info, 0, sizeof(info));
        info.fdate = (WORD)(packed >> 16);
        info.ftime = (WORD)(packed & 0xFFFF);
        FRESULT fr = f_utime(volumePath(path).c_str(), &info);
        return (FR_OK == fr) ? (int)size : fatErr(fr);
    }
    return STORAGE_ERROR_ATTR_NOT_FOUND;
}

/**
 * Reads the mode, owner and times an entry shows; the size written, or a negative
 * error code.
 */
int FatFsWrapper::getFileAttr(const char *path, uint8_t type, void *buffer, uint32_t size) {
    if (nullptr == buffer || 0 == size) {
        return PDI_ERR_INVALID_ARG;
    }
    file_info_t meta;
    pdi_err_t found = getFileMeta(path, meta);
    if (PDI_OK != found) {
        return found;
    }
    if (FILE_ATTR_PERMS == type && size >= sizeof(uint16_t)) { *(uint16_t *)buffer = meta.m_perms; return sizeof(uint16_t); }
    if (FILE_ATTR_UID == type && size >= sizeof(uint16_t))   { *(uint16_t *)buffer = meta.m_uid;   return sizeof(uint16_t); }
    if (FILE_ATTR_GID == type && size >= sizeof(uint16_t))   { *(uint16_t *)buffer = meta.m_gid;   return sizeof(uint16_t); }
    if (FILE_ATTR_CTIME == type && size >= sizeof(uint32_t)) { *(uint32_t *)buffer = meta.m_ctime; return sizeof(uint32_t); }
    if (FILE_ATTR_MTIME == type && size >= sizeof(uint32_t)) { *(uint32_t *)buffer = meta.m_mtime; return sizeof(uint32_t); }
    return STORAGE_ERROR_ATTR_NOT_FOUND;
}

/**
 * The type, size, mode and times an entry shows, m_name left alone; 0, or a
 * negative error code.
 */
pdi_err_t FatFsWrapper::getFileMeta(const char *path, file_info_t &out) {
    FILINFO info;
    bool root = false;
    FRESULT fr = statPath(path, info, root);
    if (FR_OK != fr) {
        return (pdi_err_t)fatErr(fr);
    }
    fillInfo(info, out);
    return PDI_OK;
}

/**
 * Keeps the write bits as the read-only flag, the only mode FAT carries; 0, or a
 * negative error code.
 */
int FatFsWrapper::setFilePermissions(const char *path, uint16_t perms) {
    FILINFO info;
    bool root = false;
    FRESULT fr = statPath(path, info, root);
    if (root) {
        return PDI_ERR_NOT_SUPPORTED;
    }
    if (FR_OK != fr) {
        return fatErr(fr);
    }
    BYTE readonly = (0 == (perms & FAT_PERM_WRITE_BITS)) ? (BYTE)AM_RDO : (BYTE)0;
    return fatErr(f_chmod(volumePath(path).c_str(), readonly, AM_RDO));
}

/**
 * Stamps an entry with the current time, creating an empty file when nothing is
 * there; 0, or a negative error code.
 */
pdi_err_t FatFsWrapper::touch(const char *path) {
    FILINFO info;
    bool root = false;
    FRESULT fr = statPath(path, info, root);
    if (root) {
        return PDI_OK;
    }
    if (FR_NO_FILE == fr) {
        int made = createFile(path, nullptr, 0);
        return (pdi_err_t)((made < 0) ? made : PDI_OK);
    }
    if (FR_OK != fr) {
        return (pdi_err_t)fatErr(fr);
    }
    return (pdi_err_t)fatErr(stampNow(volumePath(path)));
}

FatFsWrapper::fat_open_file_t *FatFsWrapper::openSlot(pdi_fhandle_t handle) {
    if (handle < 0 || handle >= (pdi_fhandle_t)VFS_MAX_OPEN_FILES) {
        return nullptr;
    }
    return m_openfiles[handle];
}

/**
 * Opens a file with the file_open_flag_t flags; a handle of 0 or above, or a
 * negative error code.
 */
pdi_fhandle_t FatFsWrapper::openFile(const char* path, uint8_t flags) {
    if (nullptr == path || '\0' == path[0]) {
        return (pdi_fhandle_t)STORAGE_ERROR_BAD_PATH;
    }
    int8_t slot = -1;
    for (uint8_t i = 0; i < VFS_MAX_OPEN_FILES && slot < 0; i++) {
        slot = (nullptr == m_openfiles[i]) ? (int8_t)i : slot;
    }
    if (slot < 0) {
        return (pdi_fhandle_t)STORAGE_ERROR_NODE_LIMIT;
    }

    BYTE mode = 0;
    mode |= (flags & FILE_OPEN_READ) ? (BYTE)FA_READ : (BYTE)0;
    mode |= (flags & (FILE_OPEN_WRITE | FILE_OPEN_APPEND)) ? (BYTE)FA_WRITE : (BYTE)0;
    mode |= (0 == mode) ? (BYTE)FA_READ : (BYTE)0;
    if (flags & FILE_OPEN_CREATE) {
        mode |= (flags & FILE_OPEN_TRUNCATE) ? (BYTE)FA_CREATE_ALWAYS : (BYTE)FA_OPEN_ALWAYS;
    }

    fat_open_file_t *entry = pdiutil::safe_new<fat_open_file_t>();
    if (nullptr == entry) {
        return (pdi_fhandle_t)PDI_ERR_NO_MEM;
    }
    FRESULT fr = f_open(&entry->m_file, volumePath(path).c_str(), mode);
    if (FR_OK == fr && (flags & FILE_OPEN_TRUNCATE) && !(flags & FILE_OPEN_CREATE)) {
        fr = f_truncate(&entry->m_file);
    }
    if (FR_OK == fr && (flags & FILE_OPEN_APPEND)) {
        fr = f_lseek(&entry->m_file, f_size(&entry->m_file));
    }
    if (FR_OK != fr) {
        f_close(&entry->m_file);
        pdiutil::safe_delete(entry);
        return (pdi_fhandle_t)fatErr(fr);
    }
    entry->m_append = (0 != (flags & FILE_OPEN_APPEND));
    m_openfiles[slot] = entry;
    return (pdi_fhandle_t)slot;
}

/**
 * Reads from an open handle, advancing its position; the bytes read, 0 at end of
 * file, or a negative error code.
 */
int FatFsWrapper::readFileHandle(pdi_fhandle_t handle, char* buffer, uint32_t size) {
    fat_open_file_t *entry = openSlot(handle);
    if (nullptr == entry || nullptr == buffer) {
        return PDI_ERR_INVALID_ARG;
    }
    UINT got = 0;
    FRESULT fr = (0 == size) ? FR_OK : f_read(&entry->m_file, buffer, size, &got);
    return (FR_OK == fr) ? (int)got : fatErr(fr);
}

/**
 * Writes to an open handle, at the end when opened to append; the bytes written, or
 * a negative error code.
 */
int FatFsWrapper::writeFileHandle(pdi_fhandle_t handle, const char* content, uint32_t size) {
    fat_open_file_t *entry = openSlot(handle);
    if (nullptr == entry || nullptr == content) {
        return PDI_ERR_INVALID_ARG;
    }
    if (0 == size) {
        return 0;
    }
    FRESULT fr = entry->m_append ? f_lseek(&entry->m_file, f_size(&entry->m_file)) : FR_OK;
    UINT put = 0;
    if (FR_OK == fr) {
        fr = f_write(&entry->m_file, content, size, &put);
    }
    if (FR_OK != fr) {
        return fatErr(fr);
    }
    return (put < size) ? (int)PDI_ERR_NO_SPACE : (int)put;
}

/**
 * Moves the position of an open handle; the new position, or a negative error code.
 */
int64_t FatFsWrapper::seekFile(pdi_fhandle_t handle, int64_t offset, file_seek_t whence) {
    fat_open_file_t *entry = openSlot(handle);
    if (nullptr == entry) {
        return PDI_ERR_INVALID_ARG;
    }
    int64_t base = 0;
    if (FILE_SEEK_CUR == whence) {
        base = (int64_t)f_tell(&entry->m_file);
    } else if (FILE_SEEK_END == whence) {
        base = (int64_t)f_size(&entry->m_file);
    }
    int64_t target = base + offset;
    if (target < 0 || target > (int64_t)(FSIZE_t)~(FSIZE_t)0) {
        return PDI_ERR_RANGE;
    }
    FRESULT fr = f_lseek(&entry->m_file, (FSIZE_t)target);
    return (FR_OK == fr) ? (int64_t)f_tell(&entry->m_file) : (int64_t)fatErr(fr);
}

/**
 * Pushes what the open file still holds out to the disk, leaving it open; 0, or a
 * negative error code.
 */
pdi_err_t FatFsWrapper::syncFile(pdi_fhandle_t handle) {
    fat_open_file_t *entry = openSlot(handle);
    if (nullptr == entry) {
        return PDI_ERR_INVALID_ARG;
    }
    return (pdi_err_t)fatErr(f_sync(&entry->m_file));
}

/**
 * Closes an open handle, flushing the file; 0, or a negative error code.
 */
pdi_err_t FatFsWrapper::closeFile(pdi_fhandle_t handle) {
    fat_open_file_t *entry = openSlot(handle);
    if (nullptr == entry) {
        return PDI_ERR_INVALID_ARG;
    }
    FRESULT fr = f_close(&entry->m_file);
    m_openfiles[handle] = nullptr;
    pdiutil::safe_delete(entry);
    return (pdi_err_t)fatErr(fr);
}

/**
 * How many files the volume holds open.
 */
uint16_t FatFsWrapper::openCount() const {
    uint16_t count = 0;
    for (uint8_t i = 0; i < VFS_MAX_OPEN_FILES; i++) {
        count += (nullptr != m_openfiles[i]) ? 1 : 0;
    }
    return count;
}
