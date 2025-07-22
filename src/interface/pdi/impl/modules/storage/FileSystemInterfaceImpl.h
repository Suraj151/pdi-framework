/************************* File System Interface Impl *************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

This interface defines the basic operations required for interacting with
a file system, such as creating, reading, writing to files or directories.

Author          : Suraj I.
Created Date    : 6th Apr 2025
******************************************************************************/

#ifndef _FILE_SYSTEM_INTERFACE_IMPL_H
#define _FILE_SYSTEM_INTERFACE_IMPL_H

// will use implementation of littlefs external library which implement iFileSystemInterface interface
#include "../../../../../../external/LittleFSWrapper.h"

/**
 * @class FileSystemInterfaceImpl
 *
 * This interface defines the basic operations required for interacting with
 * a file system, such as creating, reading, writing, renaming, and deleting
 * files or directories. Implementations of this interface must provide
 * concrete definitions for these methods.
 */
class FileSystemInterfaceImpl : public LittleFSWrapper {

public:
    /**
     * @brief Constructor to initialize the FileSystemInterfaceImpl.
     * @param storage Reference to an iStorageInterface implementation.
     */
    FileSystemInterfaceImpl(iStorageInterface& storage, bool defaultConfig) : 
        m_pwd(FILE_SEPARATOR),
        m_lastpwd(FILE_SEPARATOR),
        m_root(FILE_SEPARATOR),
        m_home(FILE_SEPARATOR),
        m_temp("/temp/"),
        LittleFSWrapper(storage, defaultConfig) {
    }

    /**
     * @brief Destructor for the FileSystemInterfaceImpl.
     */
    virtual ~FileSystemInterfaceImpl() {
    }

    /**
     * @brief Initializes the file system interface.
     * @return 0 on success, or a negative error code on failure.
     */
    virtual int init() override;

    /**
     * @brief Get file mime type based on file extension.
     * @param path The path of the file.
     * @return status
     */
    virtual mimetype_t getFileMimeType(const pdiutil::string &path) override;

    /**
     * @brief Gets the current working directory.
     * @return The current working directory.
     */
    virtual pdiutil::string getPWD() const override;

    /**
     * @brief Sets the present working directory.
     * @param The present working directory.
     */
    virtual bool setPWD(const char* path) override;

    /**
     * @brief Gets the last present working directory.
     * @return The last present working directory.
     */
    virtual pdiutil::string getLastPWD() const override;

    /**
     * @brief Appends a file separator to the provided path if it doesn't already end with one.
     * @param path The path to which the file separator will be appended.
     */
    virtual void appendFileSeparator(char *path) override;

    /**
     * @brief Appends a file separator to the provided path if it doesn't already end with one.
     * @param path The path to which the file separator will be appended.
     */
    virtual void appendFileSeparator(pdiutil::string &path) override;

    /**
     * @brief Get path without . and .. notations.
     * @param path The path to update.
     * @return True if the update was successful, false otherwise.
     */
    virtual bool updatePathNotations(const char* path, pdiutil::string &updatedpath) override;

    /**
     * @brief Changes the current working directory to the specified path.
     * @param path The new path to change to.
     * @return True if the directory change was successful, false otherwise.
     */
    virtual bool changeDirectory(const char* path) override;

    /**
     * @brief Gets the root directory.
     * @return The root directory.
     */
    virtual const char* getRootDirectory() const override;

    /**
     * @brief Gets the home directory.
     * @return The home directory.
     */
    virtual const char* getHomeDirectory() const override;

    /**
     * @brief Gets the temp directory.
     * @return The temp directory.
     */
    virtual const char* getTempDirectory() const override;

    /**
     * @brief Sets the home directory.
     * @return status
     */
    virtual bool setHomeDirectory(pdiutil::string &homedir) override;

    /**
     * @brief Gets the file/dir name from the path.
     * @param path The path of the file/dir.
     * @return The file/dir name.
     */
    virtual pdiutil::string basename(const char* path) override;

    /**
     * @brief Apply file size limit on filename.
     * @param name The name of the file/dir.
     */
    virtual void applyFileSizeLimit(pdiutil::string &name, uint32_t sizelimit = FILE_NAME_MAX_SIZE);

protected:
    pdiutil::string m_pwd; ///< Present working directory.
    pdiutil::string m_lastpwd; ///< Last Present working directory.
    pdiutil::string m_root; ///< Root directory of the file system.
    pdiutil::string m_home; ///< Home directory of the session.
    pdiutil::string m_temp; ///< Temporary directory for file operations.
};

#endif // _FILE_SYSTEM_INTERFACE_IMPL_H