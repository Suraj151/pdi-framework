/************************* File System Interface Impl *************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

This interface defines the basic operations required for interacting with
a file system, such as creating, reading, writing to files or directories.

Author          : Suraj I.
Created Date    : 6th Apr 2025
******************************************************************************/
#include <config/Config.h>

#if defined(ENABLE_STORAGE_SERVICE)

#include "FileSystemInterfaceImpl.h"
#include "../../../../../../external/LittleFSWrapper.cpp"
#include <helpers/StorageHelper.h>


/**
 * @brief Initializes the file system interface.
 * @return 0 on success, or a negative error code on failure.
 */
int FileSystemInterfaceImpl::init() { 

    // Initialize the file system
    lfs_config lfscfg; 
    lfscfg.read_size = 64; // Minimum read size (adjust as needed)
    lfscfg.prog_size = 64; // Minimum program size (adjust as needed)
    lfscfg.block_size = 4096; // Block size (adjust as needed)
    lfscfg.cache_size = 64; // Cache size (adjust as needed)
    lfscfg.lookahead_size = 64; // Lookahead buffer size (adjust as needed)
    lfscfg.block_cycles = 100; // Number of erase cycles before wear leveling    
    int status = initLFSConfig(&lfscfg);

    if(status == LFS_ERR_OK) {
        // Create home and temp directories if they do not exist
        // createDirectory(m_home.c_str());
        createDirectory(m_temp.c_str());
    }

    return status;
}

/**
 * @brief Get file mime type based on file extension.
 * @param path The path of the file.
 * @return status
 */
mimetype_t FileSystemInterfaceImpl::getFileMimeType(const pdiutil::string &path) {

    mimetype_t type = MIME_TYPE_MAX;

    // currently we are checking only for file extension
    // if the file exists and has an extension, we will try to find the mime type
    // if the file does not have an extension, we will return MIME_TYPE_MAX
    // if the file does not exist, we will return MIME_TYPE_MAX
    if( isFileExist(path.c_str()) && path.find_last_of('.') != pdiutil::string::npos ) {

        pdiutil::string ext = path.substr(path.find_last_of('.'));

        for (int t = 0; t < MIME_TYPE_MAX; t++){

            mimetype_t tt = static_cast<mimetype_t>(t);
            if (pdiutil::string(getMimeTypeExtension(tt)) == ext) {
                type = tt;
                break;
            }
        }            
    }

    return type;
}

/**
 * @brief Gets the present working directory.
 * @return The present working directory.
 */
pdiutil::string FileSystemInterfaceImpl::getPWD() const{
    return m_pwd;
}

/**
 * @brief Sets the present working directory.
 * @param The present working directory.
 */
bool FileSystemInterfaceImpl::setPWD(const char* path) {

    if( isDirectory(path) ){

        m_lastpwd = m_pwd;
        m_pwd = path;
        if (path[strlen(path) - 1] != FILE_SEPARATOR[0]) {
            m_pwd += FILE_SEPARATOR[0];
        }
        return true;          
    }
    return false;
}

/**
 * @brief Gets the last present working directory.
 * @return The last present working directory.
 */
pdiutil::string FileSystemInterfaceImpl::getLastPWD() const{
    return m_lastpwd;
}

/**
 * @brief Appends a file separator to the provided path if it doesn't already end with one.
 * @param path The path to which the file separator will be appended.
 */
void FileSystemInterfaceImpl::appendFileSeparator(char *path) {
    if (nullptr != path && path[strlen(path) - 1] != FILE_SEPARATOR[0]) {
        strncat(path, FILE_SEPARATOR, 1);
    }
}

/**
 * @brief Changes the current working directory to the specified path.
 * @param path The new path to change to.
 * @return True if the directory change was successful, false otherwise.
 */
bool FileSystemInterfaceImpl::changeDirectory(const char* path) {
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

            int tempi = i+1;
            int dotcount = 0;
            // Check for special cases like "." and ".."
            while (tempi < len) {
                if(path[tempi] == '.'){
                    dotcount++;
                }else if(path[tempi] == FILE_SEPARATOR[0]){
                    break;
                }else{
                    dotcount = 0;
                    break;
                }
                tempi++;
            }

            if (1 == dotcount) {
                // Handle "." (current directory)
                i += 1; // Skip "."
            }else if(2 == dotcount){
                // Handle ".." (parent directory)
                // Backtrack to the previous directory separator
                while (j > 0 && newpath[j - 1] != FILE_SEPARATOR[0]) {
                    --j;
                }
                if (j > 0) {
                    --j; // Remove the trailing separator
                }
                i += 2; // Skip ".."
            }else {
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
const char* FileSystemInterfaceImpl::getRootDirectory() const {
    return m_root.c_str();
}

/**
 * @brief Gets the home directory.
 * @return The home directory.
 */
const char* FileSystemInterfaceImpl::getHomeDirectory() const {
    return m_home.c_str();
}

/**
 * @brief Gets the temp directory.
 * @return The temp directory.
 */
const char* FileSystemInterfaceImpl::getTempDirectory() const {
    return m_temp.c_str();
}

/**
 * @brief Sets the home directory.
 * @return status
 */
bool FileSystemInterfaceImpl::setHomeDirectory(pdiutil::string &homedir) {

    if( isDirectory(homedir.c_str()) ){

        m_home = homedir;
        if (homedir.c_str()[homedir.size() - 1] != FILE_SEPARATOR[0]) {
            m_home += FILE_SEPARATOR[0];
        }
        return true;          
    }
    return false;
}

/**
 * @brief Gets the file/dir name from the path.
 * @param path The path of the file/dir.
 * @return The file/dir name.
 */
pdiutil::string FileSystemInterfaceImpl::basename(const char* path) {
    if (!path || strlen(path) == 0) {
        return pdiutil::string();
    }

    pdiutil::string pathStr(path);
    size_t lastSlash = pathStr.find_last_of(FILE_SEPARATOR);

    if (lastSlash == pdiutil::string::npos) {
        return pathStr; // No directory separator found, return the whole path
    }

    return pathStr.substr(lastSlash + 1); // Return the substring after the last separator
}

#endif