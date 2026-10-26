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

// pulled in directly because Config.h only reaches the vfs settings when the
// storage service is enabled, and this header is parsed either way
#include "config/VfsConfig.h"
#include "config/StorageConfig.h"

// Include the LittleFS library.
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
     * @param formatOnFailure Formats the storage when it holds no filesystem to mount.
     * @return 0 on success, or a negative error code on failure.
     */
    int initLFSConfig(lfs_config* lfscnfg = nullptr, bool formatOnFailure = true);

    /**
     * @brief Makes a fresh, empty file system across the whole storage.
     * @param lfscnfg Pointer to the LittleFS configuration.
     * @return 0 on success, or a negative error code on failure.
     */
    int formatLFS(lfs_config* lfscnfg);

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
     * @param offset Offset from where to read the file content.
     * @param readUntilMatchStr Pointer to the sring match to read until.
     * @param didmatchfound Optional pointer to a boolean that will be set to true if the match string was found.
     * @return The number of bytes read, or -1 on failure.
     */
    int readFile(const char* path, uint64_t size, pdiutil::function<bool(char *, uint32_t)> readbackfn, uint64_t offset = 0, const char* readUntilMatchStr=nullptr, bool *didmatchfound=nullptr) override; 

    /**
     * @brief Find the offset in file if line number given.
     * @param path The path of the file to find in.
     * @param linenumber line number to count offset till.
     * @param yield Optional callback function to yield control during long operations.
     * @return The offset found, or -1 on failure.
     */
    int64_t getOffsetFromLineNumber(const char* path, int linenumber, CallBackVoidArgFn yield = nullptr) override;

    /**
     * @brief Find the line in file if offset given.
     * @param path The path of the file to find in.
     * @param offset offset to find line number for.
     * @param yield Optional callback function to yield control during long operations.
     * @return The line number found, or -1 on failure.
     */
    int64_t getLineNumberFromOffset(const char* path, int64_t offset, CallBackVoidArgFn yield = nullptr) override;

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
    int findInFile(const char* path, const char* findStr, pdiutil::vector<uint32_t> *findindices, int maxindices = -1, int everynthindice = 1, int64_t offset = 0, CallBackVoidArgFn yield = nullptr) override;

    /**
     * @brief Get the number of lines in file.
     * @param path The path of the file.
     * @param linenumberindices A vector to store the line numbers found.
     * @param maxlinenumbers Optional to provide max limit for line number indices to get in linenumbers vector.
     * @param linenumberoffset Offset from which line number to count.
     * @param yield Optional callback function to yield control during long operations.
     * @return The number of line found, or -1 on failure.
     */
    int getLineNumbersInFile(const char* path, pdiutil::vector<uint32_t> &linenumberindices, int maxlinenumbers = -1, int linenumberoffset = 0, CallBackVoidArgFn yield = nullptr) override;

    /**
     * @brief Read the line in file.
     * @param path The path of the file.
     * @param linenumber A line number to read.
     * @param linedata A string to store the line data found.
     * @param pattern Optional pattern to match in the line.
     * @param yield Optional callback function to yield control during long operations.
     * @return number of bytes read, or -1 on failure.
     */
    int readLineInFile(const char* path, int32_t linenumber, pdiutil::string &linedata, const char* pattern = nullptr, CallBackVoidArgFn yield = nullptr) override;

    /**
     * @brief Creates a directory.
     * @param path The path of the directory to create.
     * @return 0 on success, or a negative error code on failure.
     */
    pdi_err_t createDirectory(const char* path) override;

    /**
     * @brief Deletes a directory.
     * @param path The path of the directory to delete.
     * @return 0 on success, or a negative error code on failure.
     */
    pdi_err_t deleteDirectory(const char* path) override;

    /**
     * @brief Renames a file or directory.
     * @param oldPath The current path of the file or directory.
     * @param newPath The new path of the file or directory.
     * @return 0 on success, or a negative error code on failure.
     */
    pdi_err_t rename(const char* oldPath, const char* newPath) override;

    /**
     * @brief Copies a file to a new path.
     * @param sourcePath The path of the source file.
     * @param destPath The path of the destination file.
     * @param keepgoing Optional, asked between chunks; false stops the copy and
     *        removes what it wrote, answered as PDI_ERR_ABORTED.
     * @return 0 on success, or a negative error code on failure.
     */
    pdi_err_t copyFile(const char* sourcePath, const char* destPath, CallBackVoidArgBoolRetFn keepgoing = nullptr) override;

    /**
     * @brief Deletes a file.
     * @param path The path of the file to delete.
     * @return 0 on success, or a negative error code on failure.
     */
    pdi_err_t deleteFile(const char* path) override;

    /**
     * @brief Moves a file to a new path.
     * @param oldPath The current path of the file.
     * @param newPath The new path of the file.
     * @return 0 on success, or a negative error code on failure.
     */
    pdi_err_t moveFile(const char* oldPath, const char* newPath) override;

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
     * @param pattern Optional pattern to filter files.
     * @return 0 on success, or negative on failure.
     */
    int getDirFileList(const char* path, pdiutil::vector<file_info_t>& items, const char* pattern = nullptr) override;

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

    /**
     * @brief Set a custom attribute on a file or directory.
     * @param path The path of the file or directory.
     * @param type User-defined attribute identifier (0-255).
     * @param buffer Pointer to the attribute data to write.
     * @param size Size of the attribute data in bytes.
     * @return 0 on success, or a negative error code on failure.
     */
    int setFileAttr(const char *path, uint8_t type, const void *buffer, uint32_t size);

    /**
     * @brief Get a custom attribute from a file or directory.
     * @param path The path of the file or directory.
     * @param type User-defined attribute identifier (0-255).
     * @param buffer Pointer to the buffer to receive the attribute data.
     * @param size Capacity of the buffer in bytes.
     * @return The size of the attribute on success, or a negative error code on failure.
     */
    int getFileAttr(const char *path, uint8_t type, void *buffer, uint32_t size);

    /**
     * @brief Remove a custom attribute from a file or directory.
     * @param path The path of the file or directory.
     * @param type User-defined attribute identifier (0-255).
     * @return 0 on success, or a negative error code on failure.
     */
    pdi_err_t removeFileAttr(const char *path, uint8_t type);

    int setFileOwner(const char *path, uint16_t uid, uint16_t gid) override;

    /**
     * @brief Opens a file and returns a handle backed by a real lfs file, so a
     *        caller reading or writing block by block pays one open instead of
     *        one open per block.
     * @param path The path of the file to open.
     * @param flags Combination of file_open_flag_t values.
     * @return A handle of 0 or above, or a negative error code on failure.
     */
    pdi_fhandle_t openFile(const char* path, uint8_t flags) override;

    /**
     * @brief Reads from an open handle, advancing its position.
     * @param handle Handle returned by openFile.
     * @param buffer Destination for the bytes read.
     * @param size Capacity of the buffer in bytes.
     * @return The number of bytes read, 0 at end of file, or a negative error code.
     */
    int readFileHandle(pdi_fhandle_t handle, char* buffer, uint32_t size) override;

    /**
     * @brief Writes to an open handle, advancing its position.
     * @param handle Handle returned by openFile.
     * @param content The bytes to write.
     * @param size The number of bytes to write.
     * @return The number of bytes written, or a negative error code on failure.
     */
    int writeFileHandle(pdi_fhandle_t handle, const char* content, uint32_t size) override;

    /**
     * @brief Moves the position of an open handle.
     * @param handle Handle returned by openFile.
     * @param offset Offset to move by, relative to whence.
     * @param whence Reference point for the offset.
     * @return The new position, or a negative error code on failure.
     */
    int64_t seekFile(pdi_fhandle_t handle, int64_t offset, file_seek_t whence) override;

    /**
     * @brief Pushes anything the open file still holds out to storage, leaving
     *        the handle open.
     * @param handle Handle returned by openFile.
     * @return 0 on success, or a negative error code on failure.
     */
    pdi_err_t syncFile(pdi_fhandle_t handle) override;

    /**
     * @brief Closes an open handle, flushing the file and stamping it when it
     *        was written to.
     * @param handle Handle returned by openFile.
     * @return 0 on success, or a negative error code on failure.
     */
    pdi_err_t closeFile(pdi_fhandle_t handle) override;

    /**
     * @brief How many files the filesystem holds open.
     */
    uint16_t openCount() const override;

protected:
    // Stamp ctime + mtime + perms + uid/gid on a freshly created entry. Uses
    // nowEpoch() + currentOwner() (implemented by the policy layer).
    void stampCreate(const char *path, bool isDir);

    // Refresh mtime on an existing entry.
    void stampModify(const char *path);

    /**
     * @brief Milliseconds from a free running clock, for timing a long run of
     *        flash work; a backend with no clock gives 0 and is never paused.
     */
    virtual uint32_t busyMillis() { return 0; }

    /**
     * @brief Gives the system its turn part way through a long run of flash
     *        work, such as a block being moved for wear levelling.
     */
    virtual void busyYield() {}

    /**
     * @brief Called from every flash access: after LFS_BUSY_YIELD_MS of back to
     * back accesses, gives the system its turn.
     */
    void keepAlive();

    /**
     * Unmounts what is mounted and takes the configuration given, or the minimal
     * default one, sized to the whole storage.
     */
    void fillLFSConfig(lfs_config* lfscnfg);

private:
    lfs_t m_lfs;
    lfs_config m_lfscfg;

    // set once a mount succeeds.
    bool m_mounted;

    uint32_t m_busy_from;
    uint32_t m_busy_last;

    // One open handle. Allocated on open and released on close, so an idle
    // table costs one pointer per slot. The path is kept because the close
    // stamp is path based.
    struct lfs_open_file_t {
        lfs_file_t m_file;
        pdiutil::string m_path;
        bool m_created;
        bool m_wrote;
    };

    // The handle is the index into this table, so a slot keeps its position for
    // as long as it is open.
    lfs_open_file_t *m_openfiles[VFS_MAX_OPEN_FILES];

    /**
     * @brief Resolve a handle to its open slot.
     * @param handle Handle returned by openFile.
     * @return The slot, or nullptr when the handle is not open.
     */
    lfs_open_file_t *openSlot(pdi_fhandle_t handle);

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