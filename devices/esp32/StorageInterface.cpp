/**************************** Storage Interface *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#include "StorageInterface.h"

static const char* TAG = "StorageInterface";

/**
 * @brief Constructor for the StorageInterface class.
 */
StorageInterface::StorageInterface(): m_partition(nullptr) {
    m_start_address = 0;
    m_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
    if (!m_partition) {
        // todo: log
        m_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, NULL);
        if (!m_partition) {
            // todo: log
            m_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);
        }else{
            // m_start_address = m_partition->address;
        }
    }else{
        // m_start_address = m_partition->address;
    }
}

/**
 * @brief Destructor for the StorageInterface class.
 */
StorageInterface::~StorageInterface() {}

/**
 * @brief Reads data from the storage.
 * @param address The address to read from (relative to the partition start).
 * @param buffer The buffer to store the read data.
 * @param size The number of bytes to read.
 * @return The number of bytes read or negative if failed.
 */
int64_t StorageInterface::read(uint64_t address, void* buffer, uint64_t size) {

    int64_t bytesRead = -1;

    if (m_partition) {

        esp_err_t err = esp_partition_read(m_partition, m_start_address + address, buffer, size);
        
        if (err != ESP_OK) {
    
            if( (int64_t)err > 0) bytesRead = (-1)*(int64_t)err;
        }else{
            bytesRead = size;
        }
    }

    return bytesRead; // Return the number of bytes read
}

/**
 * @brief Writes data to the storage.
 * @param address The address to write to (relative to the partition start).
 * @param buffer The data to write.
 * @param size The number of bytes to write.
 * @return The number of bytes written or negative if failed.
 */
int64_t StorageInterface::write(uint64_t address, const void* buffer, uint64_t size) {

    int64_t bytesWritten = -1;

    if (m_partition) {

        esp_err_t err = esp_partition_write(m_partition, m_start_address + address, buffer, size);
        if (err != ESP_OK) {

            if( (int64_t)err > 0) bytesWritten = (-1)*(int64_t)err;
        }else{
            bytesWritten = size;
        }
    }

    return bytesWritten; // Return the number of bytes written
}

/**
 * @brief Erases a block of storage.
 * @param address The starting address of the block to erase (relative to the partition start).
 * @param size The size of the block to erase (must be aligned to the sector size).
 * @return True if the erase operation was successful, false otherwise.
 */
bool StorageInterface::erase(uint64_t address, uint64_t size) {
    if (!m_partition) {
        return false;
    }

    // Ensure the address and size are aligned to the flash sector size
    if ((m_start_address + address) % SPI_FLASH_SEC_SIZE != 0 || size % SPI_FLASH_SEC_SIZE != 0) {
        return false;
    }

    esp_err_t err = esp_partition_erase_range(m_partition, m_start_address + address, size);
    if (err != ESP_OK) {
        return false;
    }

    return true; // Erase operation succeeded
}

/**
 * @brief Gets the total size of the storage.
 * @return The total size of the storage in bytes.
 */
uint64_t StorageInterface::size() const {
    if (!m_partition) {
        return 0;
    }

    return m_partition->size; // Return the size of the partition
}

StorageInterface __i_storage;