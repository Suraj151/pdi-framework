/****************************** Fs Db Store ***********************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Db store over a fixed size container file on storage. The container is created
once at its full length so a read never runs past the end of the file, and every
write lands in place at its offset, which keeps the mode and owner the file was
created with.

Author          : Suraj I.
Created Date    : 22nd Aug 2026
******************************************************************************/

#ifndef _FS_DBSTORE_H_
#define _FS_DBSTORE_H_

#include <config/Config.h>

#ifdef ENABLE_STORAGE_SERVICE

#include <interface/pdi/modules/database/iDbStoreInterface.h>

/**
 * @class FsDbStore
 * @brief Byte device backed by a container file on the filesystem.
 */
class FsDbStore : public iDbStoreInterface {
public:

    /**
     * @brief Constructor to initialize the FsDbStore.
     */
    FsDbStore();

    /**
     * @brief Destructor for the FsDbStore.
     */
    virtual ~FsDbStore();

    /**
     * @brief Create the container and its directory when either is missing.
     * @return PDI_OK when the container is usable.
     */
    pdi_err_t init() override;

    bool read(uint32_t offset, uint8_t *buf, uint32_t len) override;
    bool read_each(uint32_t offset, uint32_t len, uint8_t *window, uint16_t window_len,
                   pdiutil::function<void(const uint8_t *, uint32_t)> fn) override;
    bool write(uint32_t offset, const uint8_t *buf, uint32_t len) override;
    bool flush() override;
    uint32_t capacity() override;

    /**
     * @brief Whether the container is present and sized.
     */
    bool is_ready() const { return m_ready; }

private:
    bool m_ready;
};

extern FsDbStore __i_fs_dbstore;

#endif

#endif
