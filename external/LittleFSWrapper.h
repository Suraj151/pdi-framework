/***************************** LittleFS Wrapper *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 6th Apr 2025
******************************************************************************/
#ifndef _EXT_LITTLEFS_WRAPPER_H
#define _EXT_LITTLEFS_WRAPPER_H

#include "interface/pdi/modules/storage/iFileSystemInterface.h"

// Include the LittleFS library
#define LFS_NAME_MAX FILE_NAME_MAX_SIZE
#define LFS_NO_DEBUG
#define LFS_NO_WARN
#define LFS_NO_ERROR
#include "littlefs/lfs.h"

/**
 * @class LittleFSWrapper
 * @brief A C++ wrapper for the LittleFS library using iStorageInterface.
 *
 * This class provides a high-level interface for interacting with the LittleFS
 * file system while abstracting the underlying storage operations through the
 * iStorageInterface. It bridges the C-style LittleFS API with a modern C++ design.
 *
 * The wrapper initializes the LittleFS configuration, mounts the file system,
 * and provides basic file operations such as creating and reading files. It also
 * implements the required LittleFS callbacks (read, write, erase, and sync) using
 * the iStorageInterface.
 *
 * Example usage:
 * @code
 * MemoryStorage storage(1024 * 1024); // 1 MB storage
 * LittleFSWrapper fs(storage);
 * fs.createFile("/example.txt", "Hello, LittleFS!");
 * char buffer[128];
 * fs.readFile("/example.txt", buffer, sizeof(buffer));
 * @endcode
 *
 * @note The storage backend must implement the iStorageInterface.
 */
class LittleFSWrapper : public iFileSystemInterface {
public:
    /**
     * @brief Constructor to initialize the LittleFSWrapper.
     * @param storage Reference to an iStorageInterface implementation.
     * @param defaultConfig Flag to use default configuration.
     *
     * This constructor initializes the LittleFS configuration, mounts the file
     * system, and formats it if mounting fails.
     */
    LittleFSWrapper(iStorageInterface& storage, bool defaultConfig = true);

    /**
     * @brief Destructor to unmount the LittleFS file system.
     */
    virtual ~LittleFSWrapper();

    /**
     * @brief Initialize and mount the file system.
     * @param lfscnfg Pointer to the LittleFS configuration.
     * @return 0 on success, or a negative error code on failure.
     */
    int initLFSConfig(lfs_config* lfscnfg = nullptr);

    /**
     * @brief Creates a file and writes content to it.
     * @param path The path of the file to create.
     * @param content The content to write to the file.
     * @param size The size of the content to write. Default is -1 for full content.
     * @return The number of bytes written, or -1 on failure.
     */
    int createFile(const char* path, const char* content, int64_t size=-1) override;

    /**
     * @brief Edit content to a file.
     * @param path The path of the file to write to.
     * @param offset Offset from where to modify the file content.
     * @param content The content to write at offset.
     * @param size The size of the content to write.
     * @return The number of bytes written, or -1 on failure.
     */
    int editFile(const char* path, uint64_t offset, const char* content, uint32_t size) override;

    /**
     * @brief Writes content to a file.
     * @param path The path of the file to write to.
     * @param content The content to write to the file.
     * @param size The size of the content to write.
     * @param append Whether to append to the file or overwrite it. Default is false (overwrite).
     * @return The number of bytes written, or -1 on failure.
     */
    int writeFile(const char* path, const char* content, uint32_t size, bool append=false) override;

    /**
     * @brief Reads content from a file.
     * @param path The path of the file to read.
     * @param size The maximum number of bytes to read.
     * @param readbackfn callback function for readback.
     * @return The number of bytes read, or -1 on failure.
     */
    int readFile(const char* path, uint64_t size, pdiutil::function<bool(char *, uint32_t)> readbackfn) override; 

    /**
     * @brief Creates a directory.
     * @param path The path of the directory to create.
     * @return 0 on success, or a negative error code on failure.
     */
    int createDirectory(const char* path) override;

    /**
     * @brief Deletes a directory.
     * @param path The path of the directory to delete.
     * @return 0 on success, or a negative error code on failure.
     */
    int deleteDirectory(const char* path) override;

    /**
     * @brief Renames a file or directory.
     * @param oldPath The current path of the file or directory.
     * @param newPath The new path of the file or directory.
     * @return 0 on success, or a negative error code on failure.
     */
    int rename(const char* oldPath, const char* newPath) override;

    /**
     * @brief Copies a file to a new path.
     * @param sourcePath The path of the source file.
     * @param destPath The path of the destination file.
     * @return 0 on success, or a negative error code on failure.
     */
    int copyFile(const char* sourcePath, const char* destPath) override;

    /**
     * @brief Deletes a file.
     * @param path The path of the file to delete.
     * @return 0 on success, or a negative error code on failure.
     */
    int deleteFile(const char* path) override;

    /**
     * @brief Moves a file to a new path.
     * @param oldPath The current path of the file.
     * @param newPath The new path of the file.
     * @return 0 on success, or a negative error code on failure.
     */
    int moveFile(const char* oldPath, const char* newPath) override;

    /**
     * @brief Gets the size of a file.
     * @param path The path of the file.
     * @return The size of the file in bytes, or -1 on failure.
     */
    int64_t getFileSize(const char* path) override;

    /**
     * @brief Get the list of files in a provided path.
     * @param path The path of the directory to list.
     * @param items A vector to store the file information.
     * @return 0 on success, or negative on failure.
     */
    int getDirFileList(const char* path, pdiutil::vector<file_info_t>& items) override;

    /**
     * @brief Checks if a file exists at the specified path.
     * @param path The path of the file to check.
     * @return True if the file exists, false otherwise.
     */
    bool isFileExist(const char* path) override;

    /**
     * @brief Checks if a directory exists at the specified path.
     * @param path The path of the directory to check.
     * @return True if the directory exists, false otherwise.
     */
    bool isDirExist(const char* path) override;

    /**
     * @brief Checks whether path is directory or not
     * @param path The path of the directory to check.
     * @return True if the type is directory, false otherwise.
     */
    bool isDirectory(const char* path) override;

    /**
     * @brief Gets the total size of the LittleFS file system.
     * @return The total size of the file system in bytes.
     */
    uint64_t getTotalSize() override;

    /**
     * @brief Gets the used size of the LittleFS file system.
     * @return The used size of the file system in bytes.
     */
    uint64_t getUsedSize() override;

    /**
     * @brief Gets the free size of the LittleFS file system.
     * @return The free size of the file system in bytes.
     */
    uint64_t getFreeSize() override;

private:
    lfs_t m_lfs;
    lfs_config m_lfscfg;

    /**
     * @brief Callback for reading data from storage.
     * @param c The LittleFS configuration.
     * @param block The block number to read from.
     * @param offset The offset within the block.
     * @param buffer The buffer to store the read data.
     * @param size The number of bytes to read.
     * @return 0 on success, or a negative error code on failure.
     */
    static int readCallback(const struct lfs_config* c, lfs_block_t block,
                            lfs_off_t offset, void* buffer, lfs_size_t size);

    /**
     * @brief Callback for writing data to storage.
     * @param c The LittleFS configuration.
     * @param block The block number to write to.
     * @param offset The offset within the block.
     * @param buffer The data to write.
     * @param size The number of bytes to write.
     * @return 0 on success, or a negative error code on failure.
     */
    static int progCallback(const struct lfs_config* c, lfs_block_t block,
                            lfs_off_t offset, const void* buffer, lfs_size_t size);

    /**
     * @brief Callback for erasing a block of storage.
     * @param c The LittleFS configuration.
     * @param block The block number to erase.
     * @return 0 on success, or a negative error code on failure.
     */
    static int eraseCallback(const struct lfs_config* c, lfs_block_t block);

    /**
     * @brief Callback for syncing storage (no-op for most backends).
     * @param c The LittleFS configuration.
     * @return 0 on success.
     */
    static int syncCallback(const struct lfs_config* c);
};

#endif // _EXT_LITTLEFS_WRAPPER_H