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
class FileSystemInterface;

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
        m_istorage(storage),
        m_pwd("/"),
        m_root("/") {
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
     * @param size The maximum number of bytes to read.
     * @param readbackfn callback function for readback.
     * @return The number of bytes read, or -1 on failure.
     */
    virtual int readFile(const char* path, uint64_t size, pdiutil::function<bool(char *, uint32_t)> readbackfn) = 0;

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
     * @return 0 on success, or negative on failure.
     */
    virtual int getDirFileList(const char* path, pdiutil::vector<file_info_t>& items) = 0;

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
     * @brief Gets the current working directory.
     * @return The current working directory.
     */
    virtual pdiutil::string* pwd() {
        return &m_pwd;
    }

    /**
     * @brief Appends a file separator to the provided path if it doesn't already end with one.
     * @param path The path to which the file separator will be appended.
     */
    virtual void appendFileSeparator(char *path) {
        if (nullptr != path && path[strlen(path) - 1] != FILE_SEPARATOR[0]) {
            strncat(path, FILE_SEPARATOR, 1);
        }
    }

    /**
     * @brief Changes the current working directory to the specified path.
     * @param path The new path to change to.
     * @return True if the directory change was successful, false otherwise.
     */
    virtual bool changeDirectory(const char* path) {
        if (!path || !isDirectory(path)) {
            return false; // Invalid path or not a directory
        }

        const int len = strlen(path);
        char newpath[len + 1]; // Buffer for the resulting path
        memset(newpath, 0, sizeof(newpath));

        int j = 0; // Index for newpath
        int lastsepindx = 0; // Index of the last directory separator

        for (int i = 0; i < len; ++i) {
            if (path[i] == FILE_SEPARATOR[0]) {
                // Handle directory separator
                if (i + 1 < len && path[i + 1] == '.') {
                    if (i + 2 < len && path[i + 2] == '.') {
                        // Handle ".." (parent directory)
                        // Backtrack to the previous directory separator
                        while (j > 0 && newpath[j - 1] != FILE_SEPARATOR[0]) {
                            --j;
                        }
                        if (j > 0) {
                            --j; // Remove the trailing separator
                        }
                        i += 2; // Skip ".."
                    } else {
                        // Handle "." (current directory)
                        i += 1; // Skip "."
                    }
                } else {
                    // Normal directory separator
                    if (j == 0 || newpath[j - 1] != FILE_SEPARATOR[0]) {
                        newpath[j++] = FILE_SEPARATOR[0];
                    }
                }
                lastsepindx = j;
            } else {
                // Copy normal characters
                newpath[j++] = path[i];
            }
        }

        // Remove trailing separator if it's not the root directory
        if (j > 1 && newpath[j - 1] == FILE_SEPARATOR[0]) {
            newpath[--j] = '\0';
        } else {
            newpath[j] = '\0';
        }

        // Handle the case where the path is empty or only contains separators
        if (j == 0) {
            newpath[0] = FILE_SEPARATOR[0];
            newpath[1] = '\0';
        }
    
        // Validate the resulting path
        if (isDirectory(newpath)) {
            m_pwd = newpath; // Update the current working directory
            return true;
        }

        return false; // Invalid resulting path
    }

    /**
     * @brief Gets the root directory.
     * @return The root directory.
     */
    virtual const char* getRootDirectory() const {
        return m_root.c_str();
    }

protected:
    iStorageInterface& m_istorage; ///< Reference to the storage interface used for file operations.
    pdiutil::string m_pwd; ///< Current working directory.
    pdiutil::string m_root; ///< Root directory of the file system.
};

/**
 * @brief Global instance of the iFileSystemInterface class.
 * This instance is used to manage file system operations throughout the PDI stack.
 */
extern FileSystemInterface __i_fs;

#endif // _I_FILESYSTEM_INTERFACE_H