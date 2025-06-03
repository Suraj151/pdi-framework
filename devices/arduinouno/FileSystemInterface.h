/*************************** File System Interface ****************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

This interface defines the basic operations required for interacting with
a file system, such as creating, reading, writing to files or directories.

Author          : Suraj I.
Created Date    : 6th Apr 2025
******************************************************************************/

#ifndef _FILESYSTEM_INTERFACE_H
#define _FILESYSTEM_INTERFACE_H

#include "arduinouno.h"
// will use implementation of littlefs external library
#include "../../external/LittleFSWrapper.h"
#include "LoggerInterface.h"

/**
 * @class FileSystemInterface
 *
 * This interface defines the basic operations required for interacting with
 * a file system, such as creating, reading, writing, renaming, and deleting
 * files or directories. Implementations of this interface must provide
 * concrete definitions for these methods.
 */
class FileSystemInterface : public LittleFSWrapper {

public:
    /**
     * @brief Constructor to initialize the FileSystemInterface.
     * @param storage Reference to an iStorageInterface implementation.
     */
    FileSystemInterface() : LittleFSWrapper(__i_storage, false) {
    }

    /**
     * @brief Initializes the file system interface.
     * @return 0 on success, or a negative error code on failure.
     */
    int init() override{ 

        // Initialize the file system
        // lfs_config lfscfg; 
        // lfscfg.read_size = 64; // Minimum read size (adjust as needed)
        // lfscfg.prog_size = 64; // Minimum program size (adjust as needed)
        // lfscfg.block_size = FS_PHYS_BLOCK; // Block size (adjust as needed)
        // lfscfg.cache_size = 64; // Cache size (adjust as needed)
        // lfscfg.lookahead_size = 64; // Lookahead buffer size (adjust as needed)
        // lfscfg.block_cycles = 100; // Number of erase cycles before wear leveling    
        return initLFSConfig();
    }
    
    /**
     * @brief Destructor for the FileSystemInterface.
     */
    virtual ~FileSystemInterface() {
    }
};

#endif // _FILESYSTEM_INTERFACE_H