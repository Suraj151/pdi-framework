/***************************** LittleFS Wrapper *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 6th Apr 2025
******************************************************************************/

#include "LittleFSWrapper.h"

// Include LittleFS source files
#include "littlefs/lfs.c"
#include "littlefs/lfs_util.c"

/**
 * @file LittleFSWrapper.cpp
 * @brief Implementation of the LittleFSWrapper class.
 *
 * This file provides the implementation of the LittleFSWrapper class, which
 * bridges the LittleFS library with the iStorageInterface abstraction.
 */

// Constructor and destructor are implemented in the header file as inline functions.

/**
 * @brief Creates a file and writes content to it.
 * @param path The path of the file to create.
 * @param content The content to write to the file.
 * @return The number of bytes written, or -1 on failure.
 */
int LittleFSWrapper::createFile(const char* path, const char* content) {
    lfs_file_t file;
    if (lfs_file_open(&m_lfs, &file, path, LFS_O_WRONLY | LFS_O_CREAT) < 0) {
        return -1; // Failed to create file
    }
    int wsize = lfs_file_write(&m_lfs, &file, content, strlen(content));
    lfs_file_close(&m_lfs, &file);
    return wsize;
}

/**
 * @brief Reads content from a file.
 * @param path The path of the file to read.
 * @param buffer The buffer to store the read content.
 * @param size The maximum number of bytes to read.
 * @return The number of bytes read, or -1 on failure.
 */
int LittleFSWrapper::readFile(const char* path, char* buffer, uint64_t size) {
    lfs_file_t file;
    if (lfs_file_open(&m_lfs, &file, path, LFS_O_RDONLY) < 0) {
        return -1; // Failed to open file
    }
    int rsize = lfs_file_read(&m_lfs, &file, buffer, size);
    lfs_file_close(&m_lfs, &file);
    return rsize;
}

// The callback functions (readCallback, progCallback, eraseCallback, syncCallback)
// are implemented as static methods in the header file.