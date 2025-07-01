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

/**
 * @brief Constructor to initialize the LittleFSWrapper.
 * @param storage Reference to an iStorageInterface implementation.
 * @param defaultConfig Flag to use default configuration.
 *
 * This constructor initializes the LittleFS configuration, mounts the file
 * system, and formats it if mounting fails.
 */
LittleFSWrapper::LittleFSWrapper(iStorageInterface& storage, bool defaultConfig)
    : iFileSystemInterface(storage) {
    if (defaultConfig) {
        initLFSConfig();
    }
}

/**
 * @brief Destructor to unmount the LittleFS file system.
 */
LittleFSWrapper::~LittleFSWrapper() {
    lfs_unmount(&m_lfs);
}

/**
 * @brief Initialize and mount the file system.
 * @param lfscnfg Pointer to the LittleFS configuration.
 * @return 0 on success, or a negative error code on failure.
 */
int LittleFSWrapper::initLFSConfig(lfs_config *lfscnfg)
{
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

    // Mount the file system
    int ret = lfs_mount(&m_lfs, &m_lfscfg);
    if (ret != LFS_ERR_OK) {
        // Format and mount if mounting fails
        lfs_format(&m_lfs, &m_lfscfg);
        ret = lfs_mount(&m_lfs, &m_lfscfg);
    }

    return ret; // Success
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
        return fileOpenOrErr; // Failed to create file
    }
    int bytesWrittenOrErr = lfs_file_write(&m_lfs, &file, content, (size == -1) ? strlen(content) : size);
    lfs_file_close(&m_lfs, &file);
    return bytesWrittenOrErr;
}

/**
 * @brief Writes content to a file.
 * @param path The path of the file to write to.
 * @param content The content to write to the file.
 * @param size The size of the content to write.
 * @param append Whether to append to the file or overwrite it. Default is false (overwrite).
 * @return The number of bytes written, or -1 on failure.
 */
int LittleFSWrapper::writeFile(const char *path, const char *content, uint32_t size, bool append)
{
    if( size == 0 ){
        return 0;
    }

    lfs_file_t file;
    int fileOpenOrErr = lfs_file_open(&m_lfs, &file, path, LFS_O_WRONLY | LFS_O_CREAT | (append ? LFS_O_APPEND : LFS_O_TRUNC) );
    if (fileOpenOrErr < 0) {
        return fileOpenOrErr; // Failed to create file
    }
    int bytesWrittenOrErr = lfs_file_write(&m_lfs, &file, content, size);
    lfs_file_close(&m_lfs, &file);
    return bytesWrittenOrErr;
}

/**
 * @brief Reads content from a file.
 * @param path The path of the file to read.
 * @param size The maximum number of bytes to read in one loop.
 * @param readbackfn callback function for readback.
 * @return The number of bytes read, or -1 on failure.
 */
int LittleFSWrapper::readFile(const char* path, uint64_t size, pdiutil::function<bool(char *, uint32_t)> readbackfn) {
    lfs_file_t file;
    int okOrErr = lfs_file_open(&m_lfs, &file, path, LFS_O_RDONLY);
    if (okOrErr < 0) {
        return okOrErr; // Failed to open file
    }

    // Buffer to store file content
    char buffer[size];
    memset(buffer, 0, sizeof(buffer));
    int bytesReadOrErr = 0;
    
    if (nullptr != readbackfn) {

        lfs_size_t filesize = lfs_file_size(&m_lfs, &file); 
        for (lfs_size_t i = 0; i < filesize; i += size) {
            lfs_size_t chunk = lfs_min(size, filesize - i);
            okOrErr = lfs_file_read(&m_lfs, &file, buffer, chunk);
            if (okOrErr < 0) {
                bytesReadOrErr = okOrErr; // Failed to read file
                break; // Failed to read file
            }

            bytesReadOrErr += okOrErr;
            // Call the readback function with the read data. break if callback returns false
            if(readbackfn(buffer, chunk) == false) {
                break;
            }
        }
    }
    lfs_file_close(&m_lfs, &file);
    return bytesReadOrErr;
}

/**
 * @brief Creates a directory.
 * @param path The path of the directory to create.
 * @return 0 on success, or a negative error code on failure.
 */
int LittleFSWrapper::createDirectory(const char* path) {
    return lfs_mkdir(&m_lfs, path);
}

/**
 * @brief Deletes a directory.
 * @param path The path of the directory to delete.
 * @return 0 on success, or a negative error code on failure.
 */
int LittleFSWrapper::deleteDirectory(const char* path) {
    return lfs_remove(&m_lfs, path);
}

/**
 * @brief Renames a file or directory.
 * @param oldPath The current path of the file or directory.
 * @param newPath The new path of the file or directory.
 * @return 0 on success, or a negative error code on failure.
 */
int LittleFSWrapper::rename(const char* oldPath, const char* newPath) {
    return lfs_rename(&m_lfs, oldPath, newPath);
}

/**
 * @brief Copies a file to a new path.
 * @param sourcePath The path of the source file.
 * @param destPath The path of the destination file.
 * @return 0 on success, or a negative error code on failure.
 */
int LittleFSWrapper::copyFile(const char* sourcePath, const char* destPath) {
    // Buffer to store file content
    char buffer[m_lfscfg.read_size];
    memset(buffer, 0, sizeof(buffer));

    lfs_file_t sourceFile;
    int fileOpenOrErr = lfs_file_open(&m_lfs, &sourceFile, sourcePath, LFS_O_RDONLY);
    if (fileOpenOrErr < 0) {
        return fileOpenOrErr; // Failed to open source file
    }

    lfs_file_t destFile;
    fileOpenOrErr = lfs_file_open(&m_lfs, &destFile, destPath, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (fileOpenOrErr < 0) {
        return fileOpenOrErr; // Failed to open/create file
    }

    // Copy the content from source file to destination file
    lfs_size_t filesize = lfs_file_size(&m_lfs, &sourceFile); 
    for (lfs_size_t i = 0; i < filesize; i += m_lfscfg.read_size) {
        lfs_size_t chunk = lfs_min(m_lfscfg.read_size, filesize - i);
        lfs_file_read(&m_lfs, &sourceFile, buffer, chunk);
        lfs_file_write(&m_lfs, &destFile, buffer, chunk);
    }

    // Close the files
    lfs_file_close(&m_lfs, &sourceFile);
    lfs_file_close(&m_lfs, &destFile);

    return LFS_ERR_OK; // Success
}

/**
 * @brief Deletes a file.
 * @param path The path of the file to delete.
 * @return 0 on success, or a negative error code on failure.
 */
int LittleFSWrapper::deleteFile(const char *path){
    return lfs_remove(&m_lfs, path);
}

/**
 * @brief Moves a file to a new path.
 * @param oldPath The current path of the file.
 * @param newPath The new path of the file.
 * @return 0 on success, or a negative error code on failure.
 */
int LittleFSWrapper::moveFile(const char *oldPath, const char *newPath){
    return lfs_rename(&m_lfs, oldPath, newPath);
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
        return fileOpenOrErr; // Failed to open file
    }    
    int64_t size = lfs_file_size(&m_lfs, &file);
    lfs_file_close(&m_lfs, &file);
    return size;
}

/**
 * @brief Get the list of files in a provided path. 
 *        Please deallocate dirs & files char* after use.
 * @param path The path of the directory to list.
 * @param items A vector to store the file information.
 * @return 0 on success, or negative on failure.
 */
int LittleFSWrapper::getDirFileList(const char *path, pdiutil::vector<file_info_t>& items)
{
    lfs_dir_t dir;
    lfs_info info;
    
    int dirOpenOrErr = lfs_dir_open(&m_lfs, &dir, path);
    if (dirOpenOrErr < 0) {
        return dirOpenOrErr; // Failed to open directory
    }    

    while (lfs_dir_read(&m_lfs, &dir, &info) > 0) {

        char *name = new char[strlen(info.name) + 1];
        memset(name, 0, strlen(info.name) + 1);
        strcpy(name, info.name);
        items.push_back({info.type==LFS_TYPE_DIR?FILE_TYPE_DIR:FILE_TYPE_REG, info.size, name});
    }

    lfs_dir_close(&m_lfs, &dir);
    return LFS_ERR_OK;
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
    int64_t byteRead = wrapper->m_istorage.read(block * c->block_size + offset, buffer, size);
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
    int64_t byteWritten = wrapper->m_istorage.write(block * c->block_size + offset, buffer, size);
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
    bool bytesErased = wrapper->m_istorage.erase(block * c->block_size, c->block_size);
    // LogFmtI("\nlfserase callback: %d, %d", block, c->block_size);
    if( bytesErased ){
        return LFS_ERR_OK;
    }
    return LFS_ERR_IO;
}

/**
 * @brief Callback for syncing storage (no-op for most backends).
 * @param c The LittleFS configuration.
 * @return 0 on success.
 */
int LittleFSWrapper::syncCallback(const struct lfs_config* c) {
    return LFS_ERR_OK;
}
