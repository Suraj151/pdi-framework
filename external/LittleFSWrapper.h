/***************************** LittleFS Wrapper *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 6th Apr 2025
******************************************************************************/
#ifndef _LITTLEFS_WRAPPER_H
#define _LITTLEFS_WRAPPER_H

#include "interface/pdi/modules/storage/iStorageInterface.h"
#include "littlefs/lfs.h" // Include the LittleFS library

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
class LittleFSWrapper {
public:
    /**
     * @brief Constructor to initialize the LittleFSWrapper.
     * @param storage Reference to an iStorageInterface implementation.
     *
     * This constructor initializes the LittleFS configuration, mounts the file
     * system, and formats it if mounting fails.
     */
    explicit LittleFSWrapper(iStorageInterface& storage)
        : m_istorage(storage) {
        // Initialize LittleFS configuration
        m_lfscfg.read = &LittleFSWrapper::readCallback;
        m_lfscfg.prog = &LittleFSWrapper::progCallback;
        m_lfscfg.erase = &LittleFSWrapper::eraseCallback;
        m_lfscfg.sync = &LittleFSWrapper::syncCallback;

        m_lfscfg.read_size = 16; // Minimum read size (adjust as needed)
        m_lfscfg.prog_size = 16; // Minimum program size (adjust as needed)
        m_lfscfg.block_size = 4096; // Block size (adjust as needed)
        m_lfscfg.block_count = m_istorage.size() / m_lfscfg.block_size;
        m_lfscfg.cache_size = 16; // Cache size (adjust as needed)
        m_lfscfg.lookahead_size = 16; // Lookahead buffer size (adjust as needed)
        m_lfscfg.block_cycles = 500; // Number of erase cycles before wear leveling

        // Mount the file system
        if (lfs_mount(&m_lfs, &m_lfscfg) != LFS_ERR_OK) {
            // Format and mount if mounting fails
            lfs_format(&m_lfs, &m_lfscfg);
            if (lfs_mount(&m_lfs, &m_lfscfg) != LFS_ERR_OK) {
                // throw std::runtime_error("Failed to mount LittleFS");
            }
        }
    }

    /**
     * @brief Destructor to unmount the LittleFS file system.
     */
    ~LittleFSWrapper() {
        lfs_unmount(&m_lfs);
    }

    /**
     * @brief Creates a file and writes content to it.
     * @param path The path of the file to create.
     * @param content The content to write to the file.
     * @return The number of bytes written, or -1 on failure.
     */
    int createFile(const char* path, const char* content);

    /**
     * @brief Reads content from a file.
     * @param path The path of the file to read.
     * @param buffer The buffer to store the read content.
     * @param size The maximum number of bytes to read.
     * @return The number of bytes read, or -1 on failure.
     */
    int readFile(const char* path, char *buffer, uint64_t size);

private:
    iStorageInterface& m_istorage;
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
                            lfs_off_t offset, void* buffer, lfs_size_t size) {
        auto* wrapper = static_cast<LittleFSWrapper*>(c->context);
        try {
            wrapper->m_istorage.read(block * c->block_size + offset, buffer, size);
            return 0;
        } catch (...) {
            return LFS_ERR_IO;
        }
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
    static int progCallback(const struct lfs_config* c, lfs_block_t block,
                            lfs_off_t offset, const void* buffer, lfs_size_t size) {
        auto* wrapper = static_cast<LittleFSWrapper*>(c->context);
        try {
            wrapper->m_istorage.write(block * c->block_size + offset, buffer, size);
            return 0;
        } catch (...) {
            return LFS_ERR_IO;
        }
    }

    /**
     * @brief Callback for erasing a block of storage.
     * @param c The LittleFS configuration.
     * @param block The block number to erase.
     * @return 0 on success, or a negative error code on failure.
     */
    static int eraseCallback(const struct lfs_config* c, lfs_block_t block) {
        auto* wrapper = static_cast<LittleFSWrapper*>(c->context);
        try {
            wrapper->m_istorage.erase(block * c->block_size, c->block_size);
            return 0;
        } catch (...) {
            return LFS_ERR_IO;
        }
    }

    /**
     * @brief Callback for syncing storage (no-op for most backends).
     * @param c The LittleFS configuration.
     * @return 0 on success.
     */
    static int syncCallback(const struct lfs_config* c) {
        // Sync is a no-op for most m_istorage backends
        return 0;
    }
};

#endif // _LITTLEFS_WRAPPER_H