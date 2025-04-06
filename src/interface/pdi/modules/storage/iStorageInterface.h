/***************************** Storage Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 6th Apr 2025
******************************************************************************/

#ifndef _I_STORAGE_INTERFACE_H
#define _I_STORAGE_INTERFACE_H

#include <interface/interface_includes.h>

/**
 * @class iStorageInterface
 * @brief Abstract interface for storage backends.
 *
 * This interface defines the basic operations required for interacting with
 * a storage medium, such as reading, writing, erasing, and querying the size
 * of the storage. It is designed to be implemented by various storage backends,
 * such as memory, flash, or other persistent storage devices.
 *
 * Implementations of this interface must ensure that the operations are
 * performed safely and efficiently, and they should handle any hardware-specific
 * details internally.
 */
class iStorageInterface {
public:
    virtual ~iStorageInterface() = default;

    /**
     * @brief Read data from the storage.
     * @param address The starting address to read from.
     * @param buffer The buffer to store the read data.
     * @param size The number of bytes to read.
     * @throws std::runtime_error if the read operation fails.
     */
    virtual void read(uint64_t address, void* buffer, uint64_t size) const = 0;

    /**
     * @brief Write data to the storage.
     * @param address The starting address to write to.
     * @param buffer The data to write.
     * @param size The number of bytes to write.
     * @throws std::runtime_error if the write operation fails.
     */
    virtual void write(uint64_t address, const void* buffer, uint64_t size) = 0;

    /**
     * @brief Erase a block of storage.
     * @param address The starting address of the block to erase.
     * @param size The size of the block to erase.
     * @throws std::runtime_error if the erase operation fails.
     */
    virtual void erase(uint64_t address, uint64_t size) = 0;

    /**
     * @brief Get the total size of the storage.
     * @return The size of the storage in bytes.
     */
    virtual uint64_t size() const = 0;
};

#endif // _I_STORAGE_INTERFACE_H