/**************************** Storage Interface *******************************
This file is part of the pdi stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _STORAGE_INTERFACE_H
#define _STORAGE_INTERFACE_H

#include "esp8266.h"
#include <interface/pdi/modules/storage/iStorageInterface.h>
#ifdef ENABLE_CONTEXTUAL_EXECUTION
#include "threading/PreemptiveMutex.h"
#endif

/**
 * @class StorageInterface
 * @brief Implementation of the iStorageInterface.
 *
 * This class provides an interface to interact with the flash storage on the
 * ESP8266 device, implementing the required methods for reading, writing,
 * erasing, and getting the size of the storage.
 */
class StorageInterface : public iStorageInterface {
public:
    /**
     * @brief Constructor for the StorageInterface class.
     */
    StorageInterface();

    /**
     * @brief Destructor for the StorageInterface class.
     */
    ~StorageInterface();

    /**
     * @brief Reads data from the storage.
     * @param address The address to read from.
     * @param buffer The buffer to store the read data.
     * @param size The number of bytes to read.
     * @return The number of bytes read or FLASH_HAL_READ_ERROR(-1) if failed.
     */
    int64_t read(uint64_t address, void* buffer, uint64_t size) override;

    /**
     * @brief Writes data to the storage.
     * @param address The address to write to.
     * @param buffer The data to write.
     * @param size The number of bytes to write.
     * @return The number of bytes written or FLASH_HAL_WRITE_ERROR(-2) if failed.
     */
    int64_t write(uint64_t address, const void* buffer, uint64_t size) override;

    /**
     * @brief Erases a block of storage.
     * @param address The starting address of the block to erase.
     * @param size The size of the block to erase.
     * @return True if the erase operation was successful, false otherwise.
     */
    bool erase(uint64_t address, uint64_t size) override;

    /**
     * @brief Gets the total size of the storage.
     * @return The total size of the storage in bytes.
     */
    uint64_t size() const override;

private:
    uint64_t m_start_address = 0; ///< Start address of the storage

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    PreemptiveMutex m_mutex;
    #endif
};

#endif // _STORAGE_INTERFACE_H
