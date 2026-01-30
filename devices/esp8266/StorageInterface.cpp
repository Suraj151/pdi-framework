/**************************** Storage Interface *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#include "StorageInterface.h"

/**
 * @brief Constructor for the StorageInterface class.
 */
StorageInterface::StorageInterface() {
    m_start_address = FS_PHYS_ADDR;
}

/**
 * @brief Destructor for the StorageInterface class.
 */
StorageInterface::~StorageInterface() {}

/**
 * @brief Reads data from the storage.
 * @param address The address to read from.
 * @param buffer The buffer to store the read data.
 * @param size The number of bytes to read.
 * @return FLASH_HAL_OK(0) or FLASH_HAL_READ_ERROR(-1) if failed.
 */
int64_t StorageInterface::read(uint64_t address, void* buffer, uint64_t size) {
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.critical_lock();
    #endif
    int64_t ret = flash_hal_read(m_start_address + address, size, reinterpret_cast<uint8_t*>(buffer));
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.critical_unlock();
    #endif
    return ret;
}

/**
 * @brief Writes data to the storage.
 * @param address The address to write to.
 * @param buffer The data to write.
 * @param size The number of bytes to write.
 * @return FLASH_HAL_OK(0) or FLASH_HAL_WRITE_ERROR(-2) if failed.
 */
int64_t StorageInterface::write(uint64_t address, const void* buffer, uint64_t size) {
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.critical_lock();
    #endif
    int64_t ret = flash_hal_write(m_start_address + address, size, reinterpret_cast<const uint8_t*>(buffer));
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.critical_unlock();
    #endif
    return ret;
}

/**
 * @brief Erases a block of storage.
 * @param address The starting address of the block to erase.
 * @param size The size of the block to erase.
 * @return True if the erase operation was successful, false otherwise.
 */
bool StorageInterface::erase(uint64_t address, uint64_t size) {
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.critical_lock();
    #endif
    bool ret = flash_hal_erase(m_start_address + address, size) == 0; // Erase the specified block
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.critical_unlock();
    #endif
    return ret;
}

/**
 * @brief Gets the total size of the storage.
 * @return The total size of the storage in bytes.
 */
uint64_t StorageInterface::size() const {
    return FS_PHYS_SIZE; // Return the total flash chip size
}

StorageInterface __i_storage;