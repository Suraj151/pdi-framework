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
#include <interface/pdi/middlewares/iNtpInterface.h>
#include <service_provider/session/SessionManager.h>


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
 * @brief Appends a file separator to the provided path if it doesn't already end with one.
 * @param path The path to which the file separator will be appended.
 */
void FileSystemInterfaceImpl::appendFileSeparator(pdiutil::string &path) {
    if (path[path.length() - 1] != FILE_SEPARATOR[0]) {
        path += FILE_SEPARATOR;
    }
}

/**
 * @brief Get path without . and .. notations.
 * @param path The path to update.
 * @return True if the update was successful, false otherwise.
 */
bool FileSystemInterfaceImpl::updatePathNotations(const char* path, pdiutil::string &updatedpath){

    if (!path || !isDirectory(path)) {
        return false; // Invalid path or not a directory
    }

    const int len = strlen(path);
    char newpath[len + 1]; // Buffer for the resulting path
    memset(newpath, 0, len + 1);

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
        updatedpath = newpath; // Update the current working directory
        return true;
    }

    return false; // Invalid resulting path
}

/**
 * @brief Changes the current working directory to the specified path.
 * @param path The new path to change to.
 * @return True if the directory change was successful, false otherwise.
 */
bool FileSystemInterfaceImpl::changeDirectory(const char* path) {
    return updatePathNotations(path, m_pwd);
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

/**
 * @brief Apply file size limit on filename.
 * @param name The name of the file/dir.
 */
void FileSystemInterfaceImpl::applyFileSizeLimit(pdiutil::string &name, uint32_t sizelimit){

    if(name.length() > sizelimit){

        pdiutil::string fileformat;
        pdiutil::string::size_type lastDot = name.find_last_of('.');
        uint32_t keepsize = sizelimit - 1;

        if (lastDot == pdiutil::string::npos) {
        }else{
            fileformat = name.substr(lastDot);
            keepsize -= fileformat.length();
        }

        name = name.substr(0, keepsize);
        name += fileformat;
    }
}

uint32_t FileSystemInterfaceImpl::nowEpoch(){
    if( !__i_ntp.is_valid_ntptime() ){
        return 0;
    }
    return (uint32_t)__i_ntp.get_ntp_time();
}

void FileSystemInterfaceImpl::currentOwner(uint16_t &uid, uint16_t &gid){
#ifdef ENABLE_AUTH_SERVICE
    session_t *s = SessionManager::current();
    if( nullptr != s ){
        uid = s->m_uid;
        gid = s->m_gid;
        return;
    }
#endif
    uid = 0;
    gid = 0;
}

uint16_t FileSystemInterfaceImpl::currentUmask(){
    session_t *s = SessionManager::current();
    return (nullptr != s) ? s->m_umask : (uint16_t)FILE_UMASK_DEFAULT;
}

int FileSystemInterfaceImpl::getFileMeta(const char *path, file_info_t &out){
    if( !isFileExist(path) && !isDirExist(path) ){
        return -1;
    }
    bool isDir = isDirectory(path);
    out.m_type = isDir ? FILE_TYPE_DIR : FILE_TYPE_REG;
    out.m_size = isDir ? 0 : (uint64_t)getFileSize(path);
    out.m_ctime = 0;
    out.m_mtime = 0;
    out.m_perms = isDir ? (uint16_t)FILE_PERM_DEFAULT_DIR : (uint16_t)FILE_PERM_DEFAULT_FILE;
    out.m_uid = 0;
    out.m_gid = 0;
    getFileAttr(path, FILE_ATTR_CTIME, &out.m_ctime, sizeof(out.m_ctime));
    getFileAttr(path, FILE_ATTR_MTIME, &out.m_mtime, sizeof(out.m_mtime));
    getFileAttr(path, FILE_ATTR_PERMS, &out.m_perms, sizeof(out.m_perms));
    getFileAttr(path, FILE_ATTR_UID,   &out.m_uid,   sizeof(out.m_uid));
    getFileAttr(path, FILE_ATTR_GID,   &out.m_gid,   sizeof(out.m_gid));
    return 0;
}

int FileSystemInterfaceImpl::setFilePermissions(const char *path, uint16_t perms){
    return setFileAttr(path, FILE_ATTR_PERMS, &perms, sizeof(perms));
}

int FileSystemInterfaceImpl::touch(const char *path){
    if( !isFileExist(path) ){
        return createFile(path, "");
    }
    uint32_t now = nowEpoch();
    if( now == 0 ) return 0;
    return setFileAttr(path, FILE_ATTR_MTIME, &now, sizeof(now));
}

#endif