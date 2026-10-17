/**************************** Db Store Interface ******************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

This interface defines a flat byte device the database records are framed on.
Implementations decide where the bytes live, eeprom or a file on storage, and
only have to move them and say how many they hold.

Author          : Suraj I.
Created Date    : 22nd Aug 2026
******************************************************************************/

#ifndef _I_DBSTORE_INTERFACE_H_
#define _I_DBSTORE_INTERFACE_H_

#include <interface/interface_includes.h>

/**
 * @class iDbStoreInterface
 * @brief Abstract flat byte device holding the config database.
 *
 * Writes are staged and only reach the medium on flush(), so a caller can
 * update a record and its directory entry as one persisted change.
 */
class iDbStoreInterface {
public:

    /**
     * @brief Constructor to initialize the iDbStoreInterface.
     */
    iDbStoreInterface() {
    }

    /**
     * @brief Destructor for the iDbStoreInterface.
     */
    virtual ~iDbStoreInterface() {
    }

    /**
     * @brief Prepare the underlying medium for use.
     * @return 0 on success, or a negative error code on failure.
     */
    virtual pdi_err_t init() { return PDI_OK; }

    /**
     * @brief Release the underlying medium.
     *
     * A store that only holds the defaults is opened for the copy and closed
     * again, so it costs no ram while the device is running.
     *
     * @return 0 on success, or a negative error code on failure.
     */
    virtual pdi_err_t deinit() { return PDI_OK; }

    /**
     * @brief Copy a byte range out of the store.
     * @param offset First byte to read.
     * @param buf Destination buffer.
     * @param len Number of bytes to read.
     * @return True when the whole range was read.
     */
    virtual bool read(uint32_t offset, uint8_t *buf, uint32_t len) = 0;

    /**
     * @brief Hand a byte range to fn in order, one piece at a time, read through window.
     * @param offset First byte to read.
     * @param len Number of bytes to read.
     * @param window Buffer the pieces are read through.
     * @param window_len Bytes the window holds.
     * @param fn Receives each piece and its length.
     * @return True when the whole range was handed over.
     */
    virtual bool read_each(uint32_t offset, uint32_t len, uint8_t *window, uint16_t window_len,
                           pdiutil::function<void(const uint8_t *, uint32_t)> fn)
    {
        uint32_t _at = 0;

        while (_at < len)
        {
            uint32_t _chunk = (len - _at) > window_len ? window_len : (len - _at);

            if (!read(offset + _at, window, _chunk))
            {
                return false;
            }

            fn(window, _chunk);
            _at += _chunk;
        }

        return true;
    }

    /**
     * @brief Stage a byte range into the store, persisted by flush().
     * @param offset First byte to write.
     * @param buf Source buffer.
     * @param len Number of bytes to write.
     * @return True when the whole range was staged.
     */
    virtual bool write(uint32_t offset, const uint8_t *buf, uint32_t len) = 0;

    /**
     * @brief Persist everything staged since the last flush.
     * @return True when the staged bytes reached the medium.
     */
    virtual bool flush() = 0;

    /**
     * @brief Total bytes the store holds.
     */
    virtual uint32_t capacity() = 0;
};

#endif
