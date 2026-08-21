/**************************** Storage Interface *******************************
This file is part of the pdi stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 16th Aug 2026
******************************************************************************/

#include "StorageInterface.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Constructor for the StorageInterface class.
 */
StorageInterface::StorageInterface() : m_image(nullptr),
                                       m_size(MOCKDEVICE_STORAGE_SIZE),
                                       m_erase_count(0),
                                       m_write_count(0),
                                       m_read_count(0)
{
    m_image = pdiutil::safe_new_array<uint8_t>((size_t)m_size);
    if (nullptr != m_image)
    {
        memset(m_image, 0xFF, (size_t)m_size);
    }
    else
    {
        m_size = 0;
    }
}

/**
 * @brief Destructor for the StorageInterface class.
 */
StorageInterface::~StorageInterface()
{
    pdiutil::safe_delete_array(m_image);
}

bool StorageInterface::inRange(uint64_t address, uint64_t size) const
{
    if (nullptr == m_image)
    {
        return false;
    }
    return (address <= m_size) && (size <= m_size) && ((address + size) <= m_size);
}

/**
 * @brief Reads data from the storage.
 * @return The number of bytes read, or -1 when out of range.
 */
int64_t StorageInterface::read(uint64_t address, void *buffer, uint64_t size)
{
    if (nullptr == buffer || !inRange(address, size))
    {
        return -1;
    }

    memcpy(buffer, m_image + address, (size_t)size);
    m_read_count++;
    return (int64_t)size;
}

/**
 * @brief Programs data into the storage.
 *
 * Flash can only pull bits from one to zero, so the incoming byte is anded into
 * what is already there. A caller that skips the erase sees the same corruption
 * it would see on real hardware.
 *
 * @return The number of bytes written, or -1 when out of range.
 */
int64_t StorageInterface::write(uint64_t address, const void *buffer, uint64_t size)
{
    if (nullptr == buffer || !inRange(address, size))
    {
        return -1;
    }

    const uint8_t *source = (const uint8_t *)buffer;
    for (uint64_t i = 0; i < size; i++)
    {
        m_image[address + i] &= source[i];
    }

    m_write_count++;
    return (int64_t)size;
}

/**
 * @brief Erases whole blocks back to the all ones state.
 * @return True when the range is block aligned and inside the image.
 */
bool StorageInterface::erase(uint64_t address, uint64_t size)
{
    if (!inRange(address, size))
    {
        return false;
    }

    if ((address % MOCKDEVICE_STORAGE_BLOCK_SIZE) != 0 ||
        (size % MOCKDEVICE_STORAGE_BLOCK_SIZE) != 0)
    {
        return false;
    }

    memset(m_image + address, 0xFF, (size_t)size);
    m_erase_count += (uint32_t)(size / MOCKDEVICE_STORAGE_BLOCK_SIZE);
    return true;
}

uint64_t StorageInterface::size() const
{
    return m_size;
}

void StorageInterface::eraseAll()
{
    if (nullptr != m_image)
    {
        memset(m_image, 0xFF, (size_t)m_size);
    }
}

bool StorageInterface::attachBackingFile(const char *path)
{
    if (nullptr == path || nullptr == m_image)
    {
        return false;
    }

    m_backingfile = path;

    FILE *f = fopen(path, "rb");
    if (nullptr != f)
    {
        fread(m_image, 1, (size_t)m_size, f);
        fclose(f);
        return true;
    }

    flush();
    return true;
}

void StorageInterface::flush()
{
    if (m_backingfile.size() == 0 || nullptr == m_image)
    {
        return;
    }

    FILE *f = fopen(m_backingfile.c_str(), "wb");
    if (nullptr == f)
    {
        return;
    }

    fwrite(m_image, 1, (size_t)m_size, f);
    fclose(f);
}

void StorageInterface::detachBackingFile()
{
    m_backingfile.clear();
}

uint32_t StorageInterface::getEraseCount() const
{
    return m_erase_count;
}

uint32_t StorageInterface::getWriteCount() const
{
    return m_write_count;
}

uint32_t StorageInterface::getReadCount() const
{
    return m_read_count;
}

void StorageInterface::clearCounters()
{
    m_erase_count = 0;
    m_write_count = 0;
    m_read_count = 0;
}

StorageInterface __i_storage;
