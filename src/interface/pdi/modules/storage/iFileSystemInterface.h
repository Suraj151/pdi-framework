/*************************** File System Interface ****************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

This interface defines the basic operations required for interacting with
a file system, such as creating, reading, writing to files or directories.

Author          : Suraj I.
Created Date    : 6th Apr 2025
******************************************************************************/

#ifndef _I_FILESYSTEM_INTERFACE_H
#define _I_FILESYSTEM_INTERFACE_H

#include <interface/interface_includes.h>
#include "iStorageInterface.h" ///< Include the iStorageInterface header


// forward declaration of derived class for this interface
class VfsDispatcher;

/**
 * @class iFileSystemInterface
 * @brief Abstract interface for file system operations.
 *
 * This interface defines the basic operations required for interacting with
 * a file system, such as creating, reading, writing, renaming, and deleting
 * files or directories. Implementations of this interface must provide
 * concrete definitions for these methods.
 */
class iFileSystemInterface {
public:

    /**
     * @brief Constructor to initialize the iFileSystemInterface.
     * @param storage Reference to an iStorageInterface implementation.
     */
    iFileSystemInterface(iStorageInterface& storage) :
        m_istorage(storage){
    }

    /**
     * @brief Destructor for the iFileSystemInterface.
     */
    virtual ~iFileSystemInterface() {
    }

    /**
     * @brief Initializes the file system interface.
     * @return 0 on success, or a negative error code on failure.
     */
    virtual int init() { return 0; }

    /**
     * @brief Creates a file and writes content to it.
     * @param path The path of the file to create.
     * @param content The content to write to the file.
     * @param size The size of the content to write. Default is -1 for full content.
     * @return The number of bytes written, or -1 on failure.
     */
    virtual int createFile(const char* path, const char* content, int64_t size=-1) = 0;

    /**
     * @brief Edit content to a file.
     * @param path The path of the file to write to.
     * @param offset Offset from where to modify the file content.
     * @param content The content to write at offset.
     * @param size The size of the content to write.
     * @return The number of bytes written, or -1 on failure.
     */
    virtual int editFile(const char* path, uint64_t offset, const char* content, uint32_t size) = 0;

    /**
     * @brief Writes content to a file.
     * @param path The path of the file to write to.
     * @param content The content to write to the file.
     * @param size The size of the content to write.
     * @param append Whether to append to the file or overwrite it. Default is false (overwrite).
     * @return The number of bytes written, or -1 on failure.
     */
    virtual int writeFile(const char* path, const char* content, uint32_t size, bool append=false) = 0;

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
    virtual int readFile(const char* path, uint64_t size, pdiutil::function<bool(char *, uint32_t)> readbackfn, uint64_t offset = 0, const char* readUntilMatchStr=nullptr, bool *didmatchfound=nullptr) = 0;

    /**
     * @brief Find the offset in file if line number given.
     * @param path The path of the file to find in.
     * @param linenumber line number to count offset till.
     * @param yield Optional callback function to yield control during long operations.
     * @return The offset found, or -1 on failure.
     */
    virtual int64_t getOffsetFromLineNumber(const char* path, int linenumber, CallBackVoidArgFn yield = nullptr) = 0;

    /**
     * @brief Find the line in file if offset given.
     * @param path The path of the file to find in.
     * @param offset offset to find line number for.
     * @param yield Optional callback function to yield control during long operations.
     * @return The line number found, or -1 on failure.
     */
    virtual int64_t getLineNumberFromOffset(const char* path, int64_t offset, CallBackVoidArgFn yield = nullptr) = 0;

    /**
     * @brief Find the string in file.
     * @param path The path of the file to find in.
     * @param findStr Pointer to the find sring.
     * @param findindices A vector to store the indices of found occurrences.
     * @param maxindices Maximum indices of found occurrences. default unlimited i.e. -1.
     * @param everynthindice Get only nth indices of found occurrences. default every i.e. 1.
     * @param offset Offset from where to read the file content.
     * @param yield Optional callback function to yield control during long operations.
     * @return The number of finding, or -1 on failure.
     */
    virtual int findInFile(const char* path, const char* findStr, pdiutil::vector<uint32_t> *findindices, int maxindices = -1, int everynthindice = 1, int64_t offset = 0, CallBackVoidArgFn yield = nullptr) = 0;

    /**
     * @brief Get the number of lines in file.
     * @param path The path of the file.
     * @param linenumberindices A vector to store the line numbers found.
     * @param maxlinenumbers Optional to provide max limit for line number indices to get in linenumbers vector.
     * @param linenumberoffset Offset from which line number to count.
     * @param yield Optional callback function to yield control during long operations.
     * @return The number of line found, or -1 on failure.
     */
    virtual int getLineNumbersInFile(const char* path, pdiutil::vector<uint32_t> &linenumberindices, int maxlinenumbers = -1, int linenumberoffset = 0, CallBackVoidArgFn yield = nullptr) = 0;

    /**
     * @brief Read the line in file.
     * @param path The path of the file.
     * @param linenumber A line number to read.
     * @param linedata A string to store the line data found.
     * @param pattern Optional pattern to match in the line.
     * @param yield Optional callback function to yield control during long operations.
     * @return number of bytes read, or -1 on failure.
     */
    virtual int readLineInFile(const char* path, int32_t linenumber, pdiutil::string &linedata, const char* pattern = nullptr, CallBackVoidArgFn yield = nullptr) = 0;

    /**
     * @brief Creates a directory.
     * @param path The path of the directory to create.
     * @return 0 on success, or a negative error code on failure.
     */
    virtual int createDirectory(const char* path) = 0;

    /**
     * @brief Deletes a directory.
     * @param path The path of the directory to delete.
     * @return 0 on success, or a negative error code on failure.
     */
    virtual int deleteDirectory(const char* path) = 0;

    /**
     * @brief Renames a file or directory.
     * @param oldPath The current path of the file or directory.
     * @param newPath The new path of the file or directory.
     * @return 0 on success, or a negative error code on failure.
     */
    virtual int rename(const char* oldPath, const char* newPath) = 0;

    /**
     * @brief Copies a file to a new path.
     * @param sourcePath The path of the source file.
     * @param destPath The path of the destination file.
     * @return 0 on success, or a negative error code on failure.
     */
    virtual int copyFile(const char* sourcePath, const char* destPath) = 0;

    /**
     * @brief Moves a file to a new path.
     * @param oldPath The current path of the file.
     * @param newPath The new path of the file.
     * @return 0 on success, or a negative error code on failure.
     */
    virtual int moveFile(const char* oldPath, const char* newPath) = 0;

    /**
     * @brief Deletes a file.
     * @param path The path of the file to delete.
     * @return 0 on success, or a negative error code on failure.
     */
    virtual int deleteFile(const char* path) = 0;

    /**
     * @brief Gets the size of a file.
     * @param path The path of the file.
     * @return The size of the file in bytes, or -1 on failure.
     */
    virtual int64_t getFileSize(const char* path) = 0;

    /**
     * @brief Get the list of files in a provided path.
     * @param path The path of the directory to list.
     * @param items A vector to store the file information.
     * @param pattern Optional pattern to filter files.
     * @return 0 on success, or negative on failure.
     */
    virtual int getDirFileList(const char* path, pdiutil::vector<file_info_t>& items, const char* pattern = nullptr) = 0;

    /**
     * @brief Checks if a file exists at the specified path.
     * @param path The path of the file to check.
     * @return True if the file exists, false otherwise.
     */
    virtual bool isFileExist(const char* path) = 0;

    /**
     * @brief Checks if a directory exists at the specified path.
     * @param path The path of the directory to check.
     * @return True if the directory exists, false otherwise.
     */
    virtual bool isDirExist(const char* path) = 0;
    
    /**
     * @brief Checks whether path is directory or not
     * @param path The path of the directory to check.
     * @return True if the type is directory, false otherwise.
     */
    virtual bool isDirectory(const char* path) = 0;

    /**
     * @brief Gets the total size of the LittleFS file system.
     * @return The total size of the file system in bytes.
     */
    virtual uint64_t getTotalSize() = 0;

    /**
     * @brief Gets the used size of the LittleFS file system.
     * @return The used size of the file system in bytes.
     */
    virtual uint64_t getUsedSize() = 0;

    /**
     * @brief Gets the free size of the LittleFS file system.
     * @return The free size of the file system in bytes.
     */
    virtual uint64_t getFreeSize() = 0;

    /**
     * @brief Gets the present working directory.
     * @return The present working directory.
     */
    virtual pdiutil::string getPWD() const = 0;

    /**
     * @brief Sets the present working directory.
     * @param The present working directory.
     */
    virtual bool setPWD(const char* path) = 0;

    /**
     * @brief Gets the last present working directory.
     * @return The last present working directory.
     */
    virtual pdiutil::string getLastPWD() const = 0;

    /**
     * @brief Appends a file separator to the provided path if it doesn't already end with one.
     * @param path The path to which the file separator will be appended.
     */
    virtual void appendFileSeparator(char *path) = 0;

    /**
     * @brief Appends a file separator to the provided path if it doesn't already end with one.
     * @param path The path to which the file separator will be appended.
     */
    virtual void appendFileSeparator(pdiutil::string &path) = 0;

    /**
     * @brief Get path without . and .. notations.
     * @param path The path to update.
     * @return True if the update was successful, false otherwise.
     */
    virtual bool updatePathNotations(const char* path, pdiutil::string &updatedpath) = 0;

    /**
     * @brief Changes the current working directory to the specified path.
     * @param path The new path to change to.
     * @return True if the directory change was successful, false otherwise.
     */
    virtual bool changeDirectory(const char* path) = 0;

    /**
     * @brief Gets the root directory.
     * @return The root directory.
     */
    virtual const char* getRootDirectory() const = 0;

    /**
     * @brief Gets the home directory.
     * @return The home directory.
     */
    virtual const char* getHomeDirectory() const = 0;

    /**
     * @brief Gets the temp directory.
     * @return The temp directory.
     */
    virtual const char* getTempDirectory() const = 0;

    /**
     * @brief Sets the home directory.
     * @return status
     */
    virtual bool setHomeDirectory(pdiutil::string &homedir) = 0;

    /**
     * @brief Get file mime type based on file extension.
     * @param path The path of the file.
     * @return status
     */
    virtual mimetype_t getFileMimeType(const pdiutil::string &path) = 0;

    /**
     * @brief Gets the file/dir name from the path.
     * @param path The path of the file/dir.
     * @return The file/dir name.
     */
    virtual pdiutil::string basename(const char* path) = 0;

    /**
     * @brief Apply file size limit on filename.
     * @param name The name of the file/dir.
     */
    virtual void applyFileSizeLimit(pdiutil::string &name, uint32_t sizelimit = FILE_NAME_MAX_SIZE) = 0;

    /**
     * @brief Set a custom attribute on a file or directory.
     * @param path The path of the file or directory.
     * @param type User-defined attribute identifier (0-255).
     * @param buffer Pointer to the attribute data to write.
     * @param size Size of the attribute data in bytes.
     * @return 0 on success, or a negative error code on failure.
     */
    virtual int setFileAttr(const char *path, uint8_t type, const void *buffer, uint32_t size) = 0;

    /**
     * @brief Get a custom attribute from a file or directory.
     * @param path The path of the file or directory.
     * @param type User-defined attribute identifier (0-255).
     * @param buffer Pointer to the buffer to receive the attribute data.
     * @param size Capacity of the buffer in bytes.
     * @return The size of the attribute on success, or a negative error code on failure.
     */
    virtual int getFileAttr(const char *path, uint8_t type, void *buffer, uint32_t size) = 0;

    /**
     * @brief Remove a custom attribute from a file or directory.
     * @param path The path of the file or directory.
     * @param type User-defined attribute identifier (0-255).
     * @return 0 on success, or a negative error code on failure.
     */
    virtual int removeFileAttr(const char *path, uint8_t type) = 0;

    /**
     * @brief Get metadata (type, size, ctime, mtime, perms) for a single path.
     * @param path The path of the file or directory.
     * @param out file_info_t populated on success; `m_name` is left untouched.
     * @return 0 on success, or a negative error code on failure.
     */
    virtual int getFileMeta(const char *path, file_info_t &out) = 0;

    /**
     * @brief Set POSIX-style permission bits on a file or directory (advisory).
     * @param path The path of the file or directory.
     * @param perms Permission bit mask (e.g. 0644).
     * @return 0 on success, or a negative error code on failure.
     */
    virtual int setFilePermissions(const char *path, uint16_t perms) = 0;

    /**
     * @brief Set the owning uid + gid on a file or directory.
     * @param path The path of the file or directory.
     * @param uid Owning user id.
     * @param gid Owning group id.
     * @return 0 on success, or a negative error code on failure.
     */
    virtual int setFileOwner(const char *path, uint16_t uid, uint16_t gid) = 0;

    /**
     * @brief POSIX touch: create the file empty (with default perms) if it
     *        does not exist; otherwise bump its mtime to the current epoch.
     * @param path The path of the file.
     * @return 0 on success, or a negative error code on failure.
     */
    virtual int touch(const char *path) = 0;
protected:
    /**
     * @brief Get current wall-clock time as seconds since Unix epoch, or 0
     *        when the time source is not yet valid. Implemented by the policy
     *        layer (e.g. FileSystemInterfaceImpl) since it depends on the
     *        active time provider (NTP, RTC, etc.).
     */
    virtual uint32_t nowEpoch() = 0;

    /**
     * @brief Get the owning uid + gid to stamp on freshly created entries.
     *        Default is root (0/0). Implemented by the policy layer
     *        (FileSystemInterfaceImpl) to pull the current session's uid/gid.
     */
    virtual void currentOwner(uint16_t &uid, uint16_t &gid) { uid = 0; gid = 0; }

    /**
     * @brief Get the current session's umask applied at file/dir creation
     *        (perms &= ~umask). Default is 0 (no bits cleared). Implemented by
     *        the policy layer to pull the current session's umask.
     */
    virtual uint16_t currentUmask() { return 0; }

    iStorageInterface& m_istorage; ///< Reference to the storage interface used for file operations.
};

/**
 * @brief Global instance of the VFS dispatcher.
 * Every filesystem call throughout the PDI stack goes through this dispatcher,
 * which resolves the target mount by longest-prefix match and forwards to the
 * backend (LittleFS root, procfs, sysfs, tmpfs, etc.).
 */
extern VfsDispatcher __i_fs;

#endif // _I_FILESYSTEM_INTERFACE_H