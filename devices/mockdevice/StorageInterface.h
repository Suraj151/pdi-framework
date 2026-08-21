/**************************** Storage Interface *******************************
This file is part of the pdi stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 16th Aug 2026
******************************************************************************/

#ifndef _MOCKDEVICE_STORAGE_INTERFACE_H
#define _MOCKDEVICE_STORAGE_INTERFACE_H

#include "mockdevice.h"
#include <interface/pdi/modules/storage/iStorageInterface.h>

/**
 * total bytes the emulated flash offers, a whole number of 4096 byte blocks
 */
#ifndef MOCKDEVICE_STORAGE_SIZE
#define MOCKDEVICE_STORAGE_SIZE (1024 * 1024)
#endif

#ifndef MOCKDEVICE_STORAGE_BLOCK_SIZE
#define MOCKDEVICE_STORAGE_BLOCK_SIZE 4096
#endif

/**
 * @class StorageInterface
 * @brief Flash-like block storage held in host memory.
 *
 * Behaves the way real flash does rather than like plain RAM: an erase returns
 * a block to 0xFF and a program may only clear bits, so anything layered on top
 * has to erase before it rewrites. A backing file can be attached to keep an
 * image across runs.
 */
class StorageInterface : public iStorageInterface
{
public:
    StorageInterface();
    ~StorageInterface();

    int64_t read(uint64_t address, void *buffer, uint64_t size) override;
    int64_t write(uint64_t address, const void *buffer, uint64_t size) override;
    bool erase(uint64_t address, uint64_t size) override;
    uint64_t size() const override;

    /**
     * @brief Return every byte to the erased state.
     */
    void eraseAll();

    /**
     * @brief Load an image from a file and persist later writes back to it.
     *        A missing file starts erased and is created on the first flush.
     * @return true when the path was accepted.
     */
    bool attachBackingFile(const char *path);

    /**
     * @brief Write the current image out to the attached file, if any.
     */
    void flush();

    /**
     * @brief Stop persisting and keep the image in memory only.
     */
    void detachBackingFile();

    /**
     * @brief Count of block erases since the counters were cleared, which is
     *        what wear levelling shows up in.
     */
    uint32_t getEraseCount() const;

    uint32_t getWriteCount() const;
    uint32_t getReadCount() const;
    void clearCounters();

private:
    uint8_t *m_image;
    uint64_t m_size;
    pdiutil::string m_backingfile;
    uint32_t m_erase_count;
    uint32_t m_write_count;
    uint32_t m_read_count;

    bool inRange(uint64_t address, uint64_t size) const;
};

#endif // _MOCKDEVICE_STORAGE_INTERFACE_H
