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
     * @return The number of bytes written, or -1 on failure.
     */
    virtual int createFile(const char* path, const char* content) = 0;

    /**
     * @brief Reads content from a file.
     * @param path The path of the file to read.
     * @param buffer The buffer to store the read content.
     * @param size The maximum number of bytes to read.
     * @return The number of bytes read, or -1 on failure.
     */
    virtual int readFile(const char* path, char* buffer, uint64_t size) = 0;

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

    // /**
    //  * @brief Sets the current working directory.
    //  * @param path The new working directory.
    //  * @param append If true, appends the path to the current working directory.
    //  * @return 0 on success, or negative on failure.
    //  */
    // virtual int setWorkingDirectory(const char* path, bool append = false) {
    //     if (path) {
    //         if (append) {
    //             m_pwd += path;
    //         } else {
    //             m_pwd = path;
    //         }
    //         return 0;
    //     }
    //     return -1; // Invalid path
    // }

    // /**
    //  * @brief Gets the current working directory.
    //  * @return The current working directory.
    //  */
    // virtual const char* getWorkingDirectory() const {
    //     return m_pwd.c_str();
    // }

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