/***************************** LittleFS Wrapper *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

This file provides the implementation of the LittleFSWrapper class, which 
bridges the LittleFS library with the iStorageInterface abstraction.

Author          : Suraj I.
created Date    : 6th Apr 2025
******************************************************************************/

#include "LittleFSWrapper.h"
#include "interface/pdi/impl/modules/storage/FileLines.h"

// Map a LittleFS return (LFS_ERR_* negative, or a non-negative count/size) onto
// the framework error space. Non-negative values (count/size) pass through
// unchanged. A negative LFS_ERR_* is preserved exactly via the FS_LITTLEFS
// passthrough band rather than collapsed to a generic code, so both the origin
// (LittleFS) and the exact error stay traceable: lfs_err = code - PDI_ERRBASE_FS_LITTLEFS.
static int lfsToPdiErr(int rc) {
    return rc >= 0 ? rc : PDI_ERR_FROM_LFS(rc);
}

/**
 * @brief Constructor to initialize the LittleFSWrapper.
 * @param storage Reference to an iStorageInterface implementation.
 * @param defaultConfig Flag to use default configuration.
 *
 * This constructor initializes the LittleFS configuration, mounts the file
 * system, and formats it if mounting fails.
 */
LittleFSWrapper::LittleFSWrapper(iStorageInterface& storage, bool defaultConfig)
    : iFileSystemInterface(storage), m_mounted(false), m_busy_from(0), m_busy_last(0) {
    memset(&m_lfs, 0, sizeof(m_lfs));
    memset(&m_lfscfg, 0, sizeof(m_lfscfg));
    for (uint8_t i = 0; i < VFS_MAX_OPEN_FILES; i++) {
        m_openfiles[i] = nullptr;
    }
    if (defaultConfig) {
        initLFSConfig();
    }
}

/**
 * @brief Destructor to unmount the LittleFS file system.
 */
LittleFSWrapper::~LittleFSWrapper() {
    // closed directly rather than through closeFile, because the stamp it does
    // on a written file reaches virtuals the derived layer no longer provides
    for (uint8_t i = 0; i < VFS_MAX_OPEN_FILES; i++) {
        if (nullptr != m_openfiles[i]) {
            lfs_file_close(&m_lfs, &m_openfiles[i]->m_file);
            pdiutil::safe_delete(m_openfiles[i]);
        }
    }
    if (m_mounted) {
        lfs_unmount(&m_lfs);
        m_mounted = false;
    }
}

/**
 * @brief Initialize and mount the file system.
 * @param lfscnfg Pointer to the LittleFS configuration.
 * @param formatOnFailure Formats the storage when it holds no filesystem to mount.
 * @return 0 on success, or a negative error code on failure.
 */
int LittleFSWrapper::initLFSConfig(lfs_config *lfscnfg, bool formatOnFailure)
{
    fillLFSConfig(lfscnfg);

    // Mount the file system
    int ret = lfs_mount(&m_lfs, &m_lfscfg);
    if (ret != LFS_ERR_OK && formatOnFailure) {
        // Format and mount if mounting fails
        lfs_format(&m_lfs, &m_lfscfg);
        ret = lfs_mount(&m_lfs, &m_lfscfg);
    }

    m_mounted = (ret == LFS_ERR_OK);

    return lfsToPdiErr(ret); // Success
}

/**
 * @brief Makes a fresh, empty file system across the whole storage.
 * @param lfscnfg Pointer to the LittleFS configuration.
 * @return 0 on success, or a negative error code on failure.
 */
int LittleFSWrapper::formatLFS(lfs_config *lfscnfg)
{
    fillLFSConfig(lfscnfg);
    return lfsToPdiErr(lfs_format(&m_lfs, &m_lfscfg));
}

/**
 * Unmounts what is mounted and takes the configuration given, or the minimal
 * default one, sized to the whole storage.
 */
void LittleFSWrapper::fillLFSConfig(lfs_config *lfscnfg)
{
    if (m_mounted) {
        lfs_unmount(&m_lfs);
        m_mounted = false;
    }

    memset(&m_lfs, 0, sizeof(m_lfs));
    memset(&m_lfscfg, 0, sizeof(m_lfscfg));

    // Initialize LittleFS configuration
    m_lfscfg.read = &LittleFSWrapper::readCallback;
    m_lfscfg.prog = &LittleFSWrapper::progCallback;
    m_lfscfg.erase = &LittleFSWrapper::eraseCallback;
    m_lfscfg.sync = &LittleFSWrapper::syncCallback;

    if (nullptr != lfscnfg) {
        m_lfscfg.read_size = lfscnfg->read_size; // Minimum read size
        m_lfscfg.prog_size = lfscnfg->prog_size; // Minimum program size
        m_lfscfg.block_size = lfscnfg->block_size; // Block size
        m_lfscfg.cache_size = lfscnfg->cache_size; // Cache size
        m_lfscfg.lookahead_size = lfscnfg->lookahead_size; // Lookahead buffer size
        m_lfscfg.block_cycles = lfscnfg->block_cycles; // Number of erase cycles before wear leveling
    } else {
        // very minimal default sizes to start with
        m_lfscfg.read_size = 8; // Minimum read size (adjust as needed)
        m_lfscfg.prog_size = 8; // Minimum program size (adjust as needed)
        m_lfscfg.block_size = 32; // Block size (adjust as needed)
        m_lfscfg.cache_size = 8; // Cache size (adjust as needed)
        m_lfscfg.lookahead_size = 8; // Lookahead buffer size (adjust as needed)
        m_lfscfg.block_cycles = 500; // Number of erase cycles before wear leveling    
    }
    m_lfscfg.block_count = m_istorage.size() / m_lfscfg.block_size;

    m_lfscfg.context = this; // Context for lfs storage operations
}

/**
 * @brief Creates a file and writes content to it.
 * @param path The path of the file to create.
 * @param content The content to write to the file.
 * @param size The size of the content to write. Default is -1 for full content.
 * @return The number of bytes written, or -1 on failure.
 */
int LittleFSWrapper::createFile(const char* path, const char* content, int64_t size) {
    lfs_file_t file;
    int fileOpenOrErr = lfs_file_open(&m_lfs, &file, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC | LFS_O_EXCL);
    if (fileOpenOrErr < 0) {
        return lfsToPdiErr(fileOpenOrErr); // Failed to create file
    }
    int bytesWrittenOrErr = lfs_file_write(&m_lfs, &file, content, (size == -1) ? strlen(content) : size);
    lfs_file_close(&m_lfs, &file);
    if( bytesWrittenOrErr >= 0 ){
        stampCreate(path, false);
    }
    return lfsToPdiErr(bytesWrittenOrErr);
}

/**
 * @brief Edit content to a file.
 * @param path The path of the file to write to.
 * @param offset Offset from where to modify the file content.
 * @param content The content to write at offset.
 * @param size The size of the content to write.
 * @return The number of bytes written, or -1 on failure.
 */
int LittleFSWrapper::editFile(const char* path, uint64_t offset, const char* content, uint32_t size) {

    if( size == 0 ){
        return 0; // Nothing to write
    }

    lfs_file_t file;
    int fileOpenOrErr = lfs_file_open(&m_lfs, &file, path, LFS_O_WRONLY);
    if (fileOpenOrErr < 0) {
        return lfsToPdiErr(fileOpenOrErr); // Failed to open file
    }

    int64_t filesize = lfs_file_size(&m_lfs, &file);

    if (offset > (uint64_t)filesize) {
        // Move to end of file
        lfs_file_seek(&m_lfs, &file, 0, LFS_SEEK_END);

        uint8_t pad_buf[64]; // small buffer
        memset(pad_buf, 0, sizeof(pad_buf)); // fill with zeros

        uint64_t gap = offset - filesize;
        while (gap > 0) {
            uint32_t chunk = (gap > sizeof(pad_buf)) ? sizeof(pad_buf) : gap;
            int written = lfs_file_write(&m_lfs, &file, pad_buf, chunk);
            if (written <= 0) { // error or no progress
                lfs_file_close(&m_lfs, &file);
                return (written == 0) ? PDI_ERR_IO : lfsToPdiErr(written);
            }
            gap -= written;
        }
    }

    // Move to the specified offset
    if (lfs_file_seek(&m_lfs, &file, offset, LFS_SEEK_SET) < 0) {
        lfs_file_close(&m_lfs, &file);
        return STORAGE_ERROR_BACKEND; // Failed to seek to offset
    }

    int bytesWrittenOrErr = lfs_file_write(&m_lfs, &file, content, size);
    lfs_file_close(&m_lfs, &file);

    if (bytesWrittenOrErr > 0 && (uint32_t)bytesWrittenOrErr != size) {
        return PDI_ERR_IO; // Partial write
    }

    if( bytesWrittenOrErr >= 0 ){
        stampModify(path);
    }

    return lfsToPdiErr(bytesWrittenOrErr);
}

/**
 * @brief Writes content to a file.
 * @param path The path of the file to write to.
 * @param content The content to write to the file.
 * @param size The size of the content to write.
 * @param append Whether to append to the file or overwrite it. Default is false (overwrite).
 * @return The number of bytes written, or -1 on failure.
 */
int LittleFSWrapper::writeFile(const char *path, const char *content, uint32_t size, bool append){
    if( size == 0 ){
        return 0;
    }

    bool preExisted = isFileExist(path);

    lfs_file_t file;
    int fileOpenOrErr = lfs_file_open(&m_lfs, &file, path, LFS_O_WRONLY | LFS_O_CREAT | (append ? LFS_O_APPEND : LFS_O_TRUNC) );
    if (fileOpenOrErr < 0) {
        return lfsToPdiErr(fileOpenOrErr); // Failed to create file
    }
    int bytesWrittenOrErr = lfs_file_write(&m_lfs, &file, content, size);
    lfs_file_close(&m_lfs, &file);
    if( bytesWrittenOrErr >= 0 ){
        if( preExisted ){
            stampModify(path);
        } else {
            stampCreate(path, false);
        }
    }
    return lfsToPdiErr(bytesWrittenOrErr);
}

/**
 * @brief Reads content from a file.
 * @param path The path of the file to read.
 * @param size The maximum number of bytes to read in one loop.
 * @param readbackfn callback function for readback.
 * @param offset Offset from where to read the file content.
 * @param readUntilMatchStr Pointer to the sring match to read until.
 * @param didmatchfound Optional pointer to a boolean that will be set to true if the match string was found.
 * @return The number of bytes read, or -1 on failure.
 */
int LittleFSWrapper::readFile(const char* path, uint64_t size, pdiutil::function<bool(char *, uint32_t)> readbackfn, uint64_t offset, const char* readUntilMatchStr, bool *didmatchfound) {
    lfs_file_t file;
    int32_t okOrErr = lfs_file_open(&m_lfs, &file, path, LFS_O_RDONLY);
    if (okOrErr < 0) {
        return lfsToPdiErr(okOrErr); // Failed to open file
    }

    char *matchstrtempbuffer = nullptr;
    uint16_t matchstrlen = 0;
    if( nullptr != readUntilMatchStr ) {

        matchstrlen = strlen(readUntilMatchStr);

        if( matchstrlen > 0 ){
            
            matchstrtempbuffer = pdiutil::safe_new_array<char>(matchstrlen + 1);
        }
    }    

    // Buffer to store file content
    char buffer[size];
    memset(buffer, 0, size);
    int bytesReadOrErr = 0;

    if (nullptr != readbackfn) {

        lfs_size_t filesize = lfs_file_size(&m_lfs, &file);
        if( offset < filesize ){
            filesize -= offset;
            lfs_file_seek(&m_lfs, &file, offset, LFS_SEEK_SET); 
        } else {
            filesize = 0;
        }

        for (lfs_size_t i = 0; i < filesize; i += size) {

            lfs_size_t chunk = lfs_min(size, filesize - i);
            
            okOrErr = lfs_file_read(&m_lfs, &file, buffer, chunk);
            if (okOrErr < 0) {
                bytesReadOrErr = okOrErr; // Failed to read file
                break; // Failed to read file
            }

            chunk = okOrErr;
            
            bool matchfound = false;
            if( nullptr != matchstrtempbuffer ) {

                
                for (uint16_t j = 0; j < chunk; j++){

                    // Shift left by 1
                    memmove(matchstrtempbuffer, matchstrtempbuffer + 1, matchstrlen - 1);
                    matchstrtempbuffer[matchstrlen - 1] = buffer[j];

                    // Check if the read data contains the match string
                    int matchPos = __strstr(matchstrtempbuffer, readUntilMatchStr, matchstrlen);
                    if (matchPos != -1) {
                        // Found the match string, adjust chunk size to exclude it
                        chunk = matchPos > 0 ? matchPos - 1 : matchPos;
                        chunk += j;
                        // Call the readback function with the data up to the match
                        if(chunk > 0) {
                            readbackfn(buffer, chunk);
                        }
                        // Stop reading further
                        matchfound = true;
                        break;
                    }
                }
            }

            bytesReadOrErr += chunk;

            if( matchfound == true ){
                if( nullptr != didmatchfound ){
                    *didmatchfound = true;
                }
                break; // exit the for loop
            }

            // Call the readback function with the read data. break if callback returns false
            if(readbackfn(buffer, chunk) == false) {
                break;
            }
        }
    }

    pdiutil::safe_delete_array(matchstrtempbuffer);

    lfs_file_close(&m_lfs, &file);
    return lfsToPdiErr(bytesReadOrErr);
}

/**
 * @brief Set a custom attribute on a file or directory.
 * @param path The path of the file or directory.
 * @param type User-defined attribute identifier (0-255).
 * @param buffer Pointer to the attribute data to write.
 * @param size Size of the attribute data in bytes.
 * @return 0 on success, or a negative error code on failure.
 */
int LittleFSWrapper::setFileAttr(const char *path, uint8_t type, const void *buffer, uint32_t size){
    return lfsToPdiErr(lfs_setattr(&m_lfs, path, type, buffer, (lfs_size_t)size));
}

/**
 * @brief Get a custom attribute from a file or directory.
 * @param path The path of the file or directory.
 * @param type User-defined attribute identifier (0-255).
 * @param buffer Pointer to the buffer to receive the attribute data.
 * @param size Capacity of the buffer in bytes.
 * @return The size of the attribute on success, or a negative error code on failure.
 */
int LittleFSWrapper::getFileAttr(const char *path, uint8_t type, void *buffer, uint32_t size){
    return lfsToPdiErr(lfs_getattr(&m_lfs, path, type, buffer, (lfs_size_t)size));
}

/**
 * @brief Remove a custom attribute from a file or directory.
 * @param path The path of the file or directory.
 * @param type User-defined attribute identifier (0-255).
 * @return 0 on success, or a negative error code on failure.
 */
pdi_err_t LittleFSWrapper::removeFileAttr(const char *path, uint8_t type){
    return lfsToPdiErr(lfs_removeattr(&m_lfs, path, type));
}

int LittleFSWrapper::setFileOwner(const char *path, uint16_t uid, uint16_t gid){
    int r1 = lfs_setattr(&m_lfs, path, FILE_ATTR_UID, &uid, sizeof(uid));
    int r2 = lfs_setattr(&m_lfs, path, FILE_ATTR_GID, &gid, sizeof(gid));
    return (r1 < 0) ? r1 : r2;
}

LittleFSWrapper::lfs_open_file_t *LittleFSWrapper::openSlot(pdi_fhandle_t handle) {
    if (handle < 0 || handle >= (pdi_fhandle_t)VFS_MAX_OPEN_FILES) {
        return nullptr;
    }
    return m_openfiles[handle];
}

/**
 * @brief Opens a file and returns a handle backed by a real lfs file.
 * @param path The path of the file to open.
 * @param flags Combination of file_open_flag_t values.
 * @return A handle of 0 or above, or a negative error code on failure.
 */
pdi_fhandle_t LittleFSWrapper::openFile(const char *path, uint8_t flags) {

    if (nullptr == path || '\0' == path[0]) {
        return (pdi_fhandle_t)STORAGE_ERROR_BAD_PATH;
    }

    int8_t slot = -1;
    for (uint8_t i = 0; i < VFS_MAX_OPEN_FILES; i++) {
        if (nullptr == m_openfiles[i]) {
            slot = (int8_t)i;
            break;
        }
    }

    if (slot < 0) {
        return (pdi_fhandle_t)STORAGE_ERROR_NODE_LIMIT;
    }

    int lfsflags = 0;
    if ((flags & FILE_OPEN_READ) && (flags & FILE_OPEN_WRITE)) {
        lfsflags = LFS_O_RDWR;
    } else if (flags & FILE_OPEN_WRITE) {
        lfsflags = LFS_O_WRONLY;
    } else {
        lfsflags = LFS_O_RDONLY;
    }

    if (flags & FILE_OPEN_CREATE) lfsflags |= LFS_O_CREAT;
    if (flags & FILE_OPEN_TRUNCATE) lfsflags |= LFS_O_TRUNC;
    if (flags & FILE_OPEN_APPEND) lfsflags |= LFS_O_APPEND;

    bool preExisted = isFileExist(path);

    lfs_open_file_t *entry = pdiutil::safe_new<lfs_open_file_t>();
    if (nullptr == entry) {
        return (pdi_fhandle_t)PDI_ERR_NO_MEM;
    }

    int okOrErr = lfs_file_open(&m_lfs, &entry->m_file, path, lfsflags);
    if (okOrErr < 0) {
        pdiutil::safe_delete(entry);
        return (pdi_fhandle_t)lfsToPdiErr(okOrErr);
    }

    entry->m_path = path;
    entry->m_created = !preExisted;
    entry->m_wrote = false;

    m_openfiles[slot] = entry;
    return (pdi_fhandle_t)slot;
}

/**
 * @brief Reads from an open handle, advancing its position.
 * @param handle Handle returned by openFile.
 * @param buffer Destination for the bytes read.
 * @param size Capacity of the buffer in bytes.
 * @return The number of bytes read, 0 at end of file, or a negative error code.
 */
int LittleFSWrapper::readFileHandle(pdi_fhandle_t handle, char *buffer, uint32_t size) {

    lfs_open_file_t *entry = openSlot(handle);
    if (nullptr == entry || nullptr == buffer) {
        return PDI_ERR_INVALID_ARG;
    }

    if (0 == size) {
        return 0;
    }

    return lfsToPdiErr(lfs_file_read(&m_lfs, &entry->m_file, buffer, size));
}

/**
 * @brief Writes to an open handle, advancing its position.
 * @param handle Handle returned by openFile.
 * @param content The bytes to write.
 * @param size The number of bytes to write.
 * @return The number of bytes written, or a negative error code on failure.
 */
int LittleFSWrapper::writeFileHandle(pdi_fhandle_t handle, const char *content, uint32_t size) {

    lfs_open_file_t *entry = openSlot(handle);
    if (nullptr == entry || nullptr == content) {
        return PDI_ERR_INVALID_ARG;
    }

    if (0 == size) {
        return 0;
    }

    int written = lfs_file_write(&m_lfs, &entry->m_file, content, size);
    if (written >= 0) {
        entry->m_wrote = true;
    }

    return lfsToPdiErr(written);
}

/**
 * @brief Moves the position of an open handle.
 * @param handle Handle returned by openFile.
 * @param offset Offset to move by, relative to whence.
 * @param whence Reference point for the offset.
 * @return The new position, or a negative error code on failure.
 */
int64_t LittleFSWrapper::seekFile(pdi_fhandle_t handle, int64_t offset, file_seek_t whence) {

    lfs_open_file_t *entry = openSlot(handle);
    if (nullptr == entry) {
        return PDI_ERR_INVALID_ARG;
    }

    int lfswhence = LFS_SEEK_SET;
    if (FILE_SEEK_CUR == whence) {
        lfswhence = LFS_SEEK_CUR;
    } else if (FILE_SEEK_END == whence) {
        lfswhence = LFS_SEEK_END;
    }

    return lfsToPdiErr(lfs_file_seek(&m_lfs, &entry->m_file, (lfs_soff_t)offset, lfswhence));
}

/**
 * @brief Pushes anything the open file still holds out to storage, leaving the
 *        handle open.
 * @param handle Handle returned by openFile.
 * @return 0 on success, or a negative error code on failure.
 */
pdi_err_t LittleFSWrapper::syncFile(pdi_fhandle_t handle) {

    lfs_open_file_t *entry = openSlot(handle);
    if (nullptr == entry) {
        return PDI_ERR_INVALID_ARG;
    }

    return (pdi_err_t)lfsToPdiErr(lfs_file_sync(&m_lfs, &entry->m_file));
}

/**
 * @brief Closes an open handle, flushing the file and stamping it when it was
 *        written to.
 * @param handle Handle returned by openFile.
 * @return 0 on success, or a negative error code on failure.
 */
pdi_err_t LittleFSWrapper::closeFile(pdi_fhandle_t handle) {

    lfs_open_file_t *entry = openSlot(handle);
    if (nullptr == entry) {
        return PDI_ERR_INVALID_ARG;
    }

    int okOrErr = lfs_file_close(&m_lfs, &entry->m_file);

    // stamping reopens the entry by path, so it has to wait for the close
    if (okOrErr >= 0 && entry->m_wrote) {
        if (entry->m_created) {
            stampCreate(entry->m_path.c_str(), false);
        } else {
            stampModify(entry->m_path.c_str());
        }
    }

    m_openfiles[handle] = nullptr;
    pdiutil::safe_delete(entry);

    return (pdi_err_t)lfsToPdiErr(okOrErr);
}

/**
 * @brief How many files the filesystem holds open.
 */
uint16_t LittleFSWrapper::openCount() const {
    uint16_t count = 0;
    for (uint8_t i = 0; i < VFS_MAX_OPEN_FILES; i++) {
        count += (nullptr != m_openfiles[i]) ? 1 : 0;
    }
    return count;
}

void LittleFSWrapper::stampCreate(const char *path, bool isDir){
    // Perms are independent of the clock, so always write them on fresh entries.
    uint16_t perms = isDir ? (uint16_t)FILE_PERM_DEFAULT_DIR : (uint16_t)FILE_PERM_DEFAULT_FILE;
    perms &= ~currentUmask() & 0777;
    lfs_setattr(&m_lfs, path, FILE_ATTR_PERMS, &perms, sizeof(perms));

    uint16_t uid = 0, gid = 0;
    currentOwner(uid, gid);
    lfs_setattr(&m_lfs, path, FILE_ATTR_UID, &uid, sizeof(uid));
    lfs_setattr(&m_lfs, path, FILE_ATTR_GID, &gid, sizeof(gid));

    // Skip time attrs when the time source is not yet valid so we do not
    // record a misleading 0 sentinel; the next mutation with a valid clock
    // will fill them in.
    uint32_t now = nowEpoch();
    if (now == 0) return;
    lfs_setattr(&m_lfs, path, FILE_ATTR_CTIME, &now, sizeof(now));
    lfs_setattr(&m_lfs, path, FILE_ATTR_MTIME, &now, sizeof(now));
}

void LittleFSWrapper::stampModify(const char *path){
    // Skip when the time source is not yet valid so we do not clobber a
    // previously-good mtime with a 0 sentinel.
    uint32_t now = nowEpoch();
    if (now == 0) return;
    lfs_setattr(&m_lfs, path, FILE_ATTR_MTIME, &now, sizeof(now));
}

/**
 * @brief Find the string in file.
 * @param path The path of the file to find in.
 * @param findStr Pointer to the find sring.
 * @param findindices A vector to store the indices of found occurrences.
 * @param maxindices Maximum indices of found occurrences. default unlimited i.e. -1.
 * @param everynthindice Get only nth indices of found occurrences. default every i.e. 1.
 * @param offset Optional Offset from where to read the file content.
 * @param yield Optional callback function to yield control during long operations.
 * @return The number of finding, or -1 on failure.
 */
int LittleFSWrapper::findInFile(const char* path, const char* findStr, pdiutil::vector<uint32_t> *findindices, int maxindices, int everynthindice, int64_t offset, CallBackVoidArgFn yield){
    return FileLines::findInFile(*this, path, findStr, findindices, maxindices, everynthindice, offset, yield);
}

/**
 * @brief Find the offset in file if line number given.
 * @param path The path of the file to find in.
 * @param linenumber line number to count offset till.
 * @param yield Optional callback function to yield control during long operations.
 * @return The offset found, or -1 on failure.
 */
int64_t LittleFSWrapper::getOffsetFromLineNumber(const char* path, int linenumber, CallBackVoidArgFn yield){
    return FileLines::getOffsetFromLineNumber(*this, path, linenumber, yield);
}

/**
 * @brief Find the line in file if offset given.
 * @param path The path of the file to find in.
 * @param offset offset to find line number for.
 * @param yield Optional callback function to yield control during long operations.
 * @return The line number found, or -1 on failure.
 */
int64_t LittleFSWrapper::getLineNumberFromOffset(const char* path, int64_t offset, CallBackVoidArgFn yield){
    return FileLines::getLineNumberFromOffset(*this, path, offset, yield);
}

/**
 * @brief Find the number of lines in file.
 * @param path The path of the file to find in.
 * @param linenumberindices A vector to store the line numbers found.
 * @param maxlinenumbers Optional to provide max limit for line number indices to get in linenumbers vector.
 * @param linenumberoffset Offset from which line number to count.
 * @param yield Optional callback function to yield control during long operations.
 * @return The number of line found, or -1 on failure.
 */
int LittleFSWrapper::getLineNumbersInFile(const char* path, pdiutil::vector<uint32_t> &linenumberindices, int maxlinenumbers, int linenumberoffset, CallBackVoidArgFn yield){
    return FileLines::getLineNumbersInFile(*this, path, linenumberindices, maxlinenumbers, linenumberoffset, yield);
}

/**
 * @brief Read the line in file.
 * @param path The path of the file.
 * @param linenumber A line number to read. line number starts from 0. Supports negative line number to read from end.
 * @param linedata A string to store the line data found.
 * @param pattern Optional pattern to match in the line.
 * @param yield Optional callback function to yield control during long operations.
 * @return number of bytes read, or -1 on failure.
 */
int LittleFSWrapper::readLineInFile(const char* path, int32_t linenumber, pdiutil::string &linedata, const char* pattern, CallBackVoidArgFn yield){
    return FileLines::readLineInFile(*this, path, linenumber, linedata, pattern, yield);
}

/**
 * @brief Creates a directory.
 * @param path The path of the directory to create.
 * @return 0 on success, or a negative error code on failure.
 */
pdi_err_t LittleFSWrapper::createDirectory(const char* path) {
    if (!path || path[0] == '\0') return PDI_ERR_INVALID_ARG;

    char temp[LFS_NAME_MAX*2]; // Adjust size as needed
    size_t len = strlen(path);
    if (len >= sizeof(temp)) return STORAGE_ERROR_NAME_TOO_LONG;

    int res = 0;
    size_t i = 0;
    size_t last = 0;

    // Skip leading slash
    if (path[0] == '/') last = 1;

    while (i <= len) {
        if (path[i] == '/' || path[i] == '\0') {
            if (i > last) {
                memcpy(temp, path, i);
                temp[i] = '\0';
                res = lfs_mkdir(&m_lfs, temp);
                if (res != 0 && res != LFS_ERR_EXIST) return lfsToPdiErr(res);
                if (res == 0) {
                    stampCreate(temp, true);
                }
            }
            last = i + 1;
        }
        i++;
    }
    return 0;
}

/**
 * @brief Deletes a directory.
 * @param path The path of the directory to delete.
 * @return 0 on success, or a negative error code on failure.
 */
pdi_err_t LittleFSWrapper::deleteDirectory(const char* path) {
    lfs_dir_t dir;
    lfs_info info;

    int dirOpenOrErr = lfs_dir_open(&m_lfs, &dir, path);
    if (dirOpenOrErr < 0) {
        return lfsToPdiErr(dirOpenOrErr); // Failed to open directory
    }

    while (lfs_dir_read(&m_lfs, &dir, &info) > 0) {
        // Skip "." and ".."
        if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0) continue;

        // Build full child path
        char childPath[256];
        snprintf(childPath, sizeof(childPath), "%s/%s", path, info.name);

        if (info.type == LFS_TYPE_DIR) {
            // Recursively delete subdirectory
            int res = deleteDirectory(childPath);
            if (res < 0) {
                lfs_dir_close(&m_lfs, &dir);
                return res;
            }
        } else {
            // Delete file
            int res = lfs_remove(&m_lfs, childPath);
            if (res < 0) {
                lfs_dir_close(&m_lfs, &dir);
                return lfsToPdiErr(res);
            }
        }
    }
    lfs_dir_close(&m_lfs, &dir);

    // Now delete the (now empty) directory itself
    return lfsToPdiErr(lfs_remove(&m_lfs, path));
}

/**
 * @brief Renames a file or directory.
 * @param oldPath The current path of the file or directory.
 * @param newPath The new path of the file or directory.
 * @return 0 on success, or a negative error code on failure.
 */
pdi_err_t LittleFSWrapper::rename(const char* oldPath, const char* newPath) {
    return lfsToPdiErr(lfs_rename(&m_lfs, oldPath, newPath));
}

/**
 * @brief Copies a file to a new path.
 * @param sourcePath The path of the source file.
 * @param destPath The path of the destination file.
 * @param keepgoing Optional, asked between chunks; false stops the copy and
 *        removes what it wrote, answered as PDI_ERR_ABORTED.
 * @return 0 on success, or a negative error code on failure.
 */
pdi_err_t LittleFSWrapper::copyFile(const char* sourcePath, const char* destPath, CallBackVoidArgBoolRetFn keepgoing) {
    // Buffer to store file content
    char buffer[m_lfscfg.read_size];
    memset(buffer, 0, sizeof(buffer));

    lfs_file_t sourceFile;
    int fileOpenOrErr = lfs_file_open(&m_lfs, &sourceFile, sourcePath, LFS_O_RDONLY);
    if (fileOpenOrErr < 0) {
        return lfsToPdiErr(fileOpenOrErr); // Failed to open source file
    }

    lfs_file_t destFile;
    fileOpenOrErr = lfs_file_open(&m_lfs, &destFile, destPath, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (fileOpenOrErr < 0) {
        return lfsToPdiErr(fileOpenOrErr); // Failed to open/create file
    }

    // Copy the content from source file to destination file
    lfs_size_t filesize = lfs_file_size(&m_lfs, &sourceFile);
    int copyOrErr = 0;
    bool stopped = false;
    for (lfs_size_t i = 0; i < filesize; i += m_lfscfg.read_size) {
        lfs_size_t chunk = lfs_min(m_lfscfg.read_size, filesize - i);
        copyOrErr = lfs_file_read(&m_lfs, &sourceFile, buffer, chunk);
        if (copyOrErr < 0) break;
        copyOrErr = lfs_file_write(&m_lfs, &destFile, buffer, chunk);
        if (copyOrErr < 0) break;
        stopped = (nullptr != keepgoing) && !keepgoing();
        if (stopped) break;
    }

    // Close the files
    lfs_file_close(&m_lfs, &sourceFile);
    lfs_file_close(&m_lfs, &destFile);

    if (stopped) {
        lfs_remove(&m_lfs, destPath);
        return PDI_ERR_ABORTED;
    }

    if (copyOrErr < 0) {
        lfs_remove(&m_lfs, destPath);
        return lfsToPdiErr(copyOrErr);
    }

    // Fresh entry: stamp ctime/mtime, then carry over source perms if present.
    stampCreate(destPath, false);
    uint16_t srcPerms = 0;
    if (lfs_getattr(&m_lfs, sourcePath, FILE_ATTR_PERMS, &srcPerms, sizeof(srcPerms)) == (int)sizeof(srcPerms)) {
        lfs_setattr(&m_lfs, destPath, FILE_ATTR_PERMS, &srcPerms, sizeof(srcPerms));
    }

    return PDI_OK; // Success
}

/**
 * @brief Deletes a file.
 * @param path The path of the file to delete.
 * @return 0 on success, or a negative error code on failure.
 */
pdi_err_t LittleFSWrapper::deleteFile(const char *path){
    return lfsToPdiErr(lfs_remove(&m_lfs, path));
}

/**
 * @brief Moves a file to a new path.
 * @param oldPath The current path of the file.
 * @param newPath The new path of the file.
 * @return 0 on success, or a negative error code on failure.
 */
pdi_err_t LittleFSWrapper::moveFile(const char *oldPath, const char *newPath){
    return lfsToPdiErr(lfs_rename(&m_lfs, oldPath, newPath));
}

/**
 * @brief Gets the size of a file.
 * @param path The path of the file.
 * @return The size of the file in bytes, or -1 on failure.
 */
int64_t LittleFSWrapper::getFileSize(const char *path) {
    lfs_file_t file;
    int fileOpenOrErr = lfs_file_open(&m_lfs, &file, path, LFS_O_RDONLY);
    if (fileOpenOrErr < 0) {
        return lfsToPdiErr(fileOpenOrErr); // Failed to open file
    }    
    int64_t size = lfs_file_size(&m_lfs, &file);
    lfs_file_close(&m_lfs, &file);
    return size < 0 ? lfsToPdiErr((int)size) : size;
}

/**
 * @brief Get the list of files in a provided path. 
 *        Please deallocate dirs & files char* after use.
 * @param path The path of the directory to list.
 * @param items A vector to store the file information.
 * @param pattern Optional pattern to filter files.
 * @return 0 on success, or negative on failure.
 */
int LittleFSWrapper::getDirFileList(const char *path, pdiutil::vector<file_info_t>& items, const char* pattern)
{
    lfs_dir_t dir;
    lfs_info info;
    
    int dirOpenOrErr = lfs_dir_open(&m_lfs, &dir, path);
    if (dirOpenOrErr < 0) {
        return lfsToPdiErr(dirOpenOrErr); // Failed to open directory
    }    

    size_t basepathlen = strlen(path);
    bool needsep = (basepathlen > 0 && path[basepathlen - 1] != '/');

    while (lfs_dir_read(&m_lfs, &dir, &info) > 0) {

        char *name = pdiutil::safe_new_array<char>(strlen(info.name) + 1);
        if( nullptr == name ){
            continue; // not enough heap for this entry
        }
        strcpy(name, info.name);

        if( nullptr != pattern && strlen(pattern) > 0 ){
            if( strlen(pattern) > strlen(name) ||
                __are_arrays_equal(name, (char*)pattern, strlen(pattern)) == false ){
                pdiutil::safe_delete_array(name);
                continue; // pattern not matched, skip this file
            }
        }

        file_info_t entry;
        entry.m_type = (info.type == LFS_TYPE_DIR) ? FILE_TYPE_DIR : FILE_TYPE_REG;
        entry.m_size = info.size;
        entry.m_name = name;
        entry.m_ctime = 0;
        entry.m_mtime = 0;
        entry.m_perms = (info.type == LFS_TYPE_DIR) ? (uint16_t)FILE_PERM_DEFAULT_DIR : (uint16_t)FILE_PERM_DEFAULT_FILE;
        entry.m_uid = 0;
        entry.m_gid = 0;

        // Skip attr lookup for "." and ".." (they are not real distinct entries).
        if (strcmp(info.name, ".") != 0 && strcmp(info.name, "..") != 0) {
            char childpath[LFS_NAME_MAX * 2];
            size_t namelen = strlen(info.name);
            if (basepathlen + (needsep ? 1 : 0) + namelen < sizeof(childpath)) {
                memcpy(childpath, path, basepathlen);
                size_t off = basepathlen;
                if (needsep) { childpath[off++] = '/'; }
                memcpy(childpath + off, info.name, namelen);
                childpath[off + namelen] = '\0';
                lfs_getattr(&m_lfs, childpath, FILE_ATTR_CTIME, &entry.m_ctime, sizeof(entry.m_ctime));
                lfs_getattr(&m_lfs, childpath, FILE_ATTR_MTIME, &entry.m_mtime, sizeof(entry.m_mtime));
                lfs_getattr(&m_lfs, childpath, FILE_ATTR_PERMS, &entry.m_perms, sizeof(entry.m_perms));
                lfs_getattr(&m_lfs, childpath, FILE_ATTR_UID,   &entry.m_uid,   sizeof(entry.m_uid));
                lfs_getattr(&m_lfs, childpath, FILE_ATTR_GID,   &entry.m_gid,   sizeof(entry.m_gid));
            }
        }

        items.push_back(entry);
    }

    lfs_dir_close(&m_lfs, &dir);
    return PDI_OK;
}

/**
 * @brief Checks if a file exists at the specified path.
 * @param path The path of the file to check.
 * @return True if the file exists, false otherwise.
 */
bool LittleFSWrapper::isFileExist(const char *path)
{
    lfs_file_t file;
    int fileOpenOrErr = lfs_file_open(&m_lfs, &file, path, LFS_O_RDONLY);
    if (fileOpenOrErr < 0) {
        return false;
    }
    lfs_file_close(&m_lfs, &file);
    return true;
}

/**
 * @brief Checks if a directory exists at the specified path.
 * @param path The path of the directory to check.
 * @return True if the directory exists, false otherwise.
 */
bool LittleFSWrapper::isDirExist(const char *path)
{
    lfs_dir_t dir;
    int dirOpenOrErr = lfs_dir_open(&m_lfs, &dir, path);
    if (dirOpenOrErr < 0) {
        return false;
    }
    lfs_dir_close(&m_lfs, &dir);
    return true;
}

/**
 * @brief Checks whether path is directory or not
 * @param path The path of the directory to check.
 * @return True if the type is directory, false otherwise.
 */
bool LittleFSWrapper::isDirectory(const char *path)
{
    return isDirExist(path);
}

/**
 * @brief Gets the total size of the LittleFS file system.
 * @return The total size of the file system in bytes.
 */
uint64_t LittleFSWrapper::getTotalSize() {
    return m_lfscfg.block_size * m_lfscfg.block_count;
}

/**
 * @brief Gets the used size of the LittleFS file system.
 * @return The used size of the file system in bytes.
 */
uint64_t LittleFSWrapper::getUsedSize() {
    lfs_info info;
    int32_t usedBlocks = lfs_fs_size(&m_lfs);

    // Iterate through all files and directories to calculate used blocks
    // lfs_dir_t dir;
    // if (lfs_dir_open(&m_lfs, &dir, "/") == 0) {
    //     while (lfs_dir_read(&m_lfs, &dir, &info) > 0) {
    //         if (info.type == LFS_TYPE_REG || info.type == LFS_TYPE_DIR) {
    //             usedBlocks += (info.size + m_lfscfg.block_size - 1) / m_lfscfg.block_size;
    //         }
    //     }
    //     lfs_dir_close(&m_lfs, &dir);
    // }

    return usedBlocks * m_lfscfg.block_size;
}

/**
 * @brief Gets the free size of the LittleFS file system.
 * @return The free size of the file system in bytes.
 */
uint64_t LittleFSWrapper::getFreeSize() {
    uint64_t totalSize = getTotalSize();
    uint64_t usedSize = getUsedSize();
    return totalSize > usedSize ? totalSize - usedSize : 0;
}

/**
 * @brief Callback for reading data from storage.
 * @param c The LittleFS configuration.
 * @param block The block number to read from.
 * @param offset The offset within the block.
 * @param buffer The buffer to store the read data.
 * @param size The number of bytes to read.
 * @return 0 on success, or a negative error code on failure.
 */
int LittleFSWrapper::readCallback(const struct lfs_config *c, lfs_block_t block,
                                  lfs_off_t offset, void *buffer, lfs_size_t size)
{
    auto* wrapper = static_cast<LittleFSWrapper*>(c->context);
    wrapper->keepAlive();
    int64_t byteRead = wrapper->m_istorage.read((uint64_t)block * c->block_size + offset, buffer, size);
    // LogFmtI("\nlfsread callback: %d, %d, %d, %d", block, c->block_size, offset, byteRead);
    if( byteRead < 0 ){
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

/**
 * @brief Callback for writing data to storage.
 * @param c The LittleFS configuration.
 * @param block The block number to write to.
 * @param offset The offset within the block.
 * @param buffer The data to write.
 * @param size The number of bytes to write.
 * @return 0 on success, or a negative error code on failure.
 */
int LittleFSWrapper::progCallback(const struct lfs_config* c, lfs_block_t block,
                                  lfs_off_t offset, const void* buffer, lfs_size_t size) {
    auto* wrapper = static_cast<LittleFSWrapper*>(c->context);
    wrapper->keepAlive();
    int64_t byteWritten = wrapper->m_istorage.write((uint64_t)block * c->block_size + offset, buffer, size);
    // LogFmtI("\nlfsprog callback: %d, %d, %d, %d", block, c->block_size, offset, byteWritten);
    if( byteWritten < 0 ){
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

/**
 * @brief Callback for erasing a block of storage.
 * @param c The LittleFS configuration.
 * @param block The block number to erase.
 * @return 0 on success, or a negative error code on failure.
 */
int LittleFSWrapper::eraseCallback(const struct lfs_config* c, lfs_block_t block) {
    auto* wrapper = static_cast<LittleFSWrapper*>(c->context);
    wrapper->keepAlive();
    bool bytesErased = wrapper->m_istorage.erase((uint64_t)block * c->block_size, c->block_size);
    // LogFmtI("\nlfserase callback: %d, %d", block, c->block_size);
    if( bytesErased ){
        return LFS_ERR_OK;
    }
    return LFS_ERR_IO;
}

/**
 * @brief Called from every flash access: after LFS_BUSY_YIELD_MS of back to back
 * accesses, gives the system its turn.
 */
void LittleFSWrapper::keepAlive() {
    uint32_t now = busyMillis();
    if ((uint32_t)(now - m_busy_last) > LFS_BUSY_YIELD_MS) {
        m_busy_from = now;
    } else if ((uint32_t)(now - m_busy_from) >= LFS_BUSY_YIELD_MS) {
        busyYield();
        now = busyMillis();
        m_busy_from = now;
    }
    m_busy_last = now;
}

/**
 * @brief Callback for syncing storage (no-op for most backends).
 * @param c The LittleFS configuration.
 * @return 0 on success.
 */
int LittleFSWrapper::syncCallback(const struct lfs_config* c) {
    return LFS_ERR_OK;
}
