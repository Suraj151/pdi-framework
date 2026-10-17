/****************************** Fs Db Store ***********************************
This file is part of the PDI Stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 22nd Aug 2026
******************************************************************************/

#include "FsDbStore.h"

#ifdef ENABLE_STORAGE_SERVICE

#include <interface/pdi.h>

/**
 * Constructor
 */
FsDbStore::FsDbStore() : m_ready(false)
{
}

/**
 * Destructor
 */
FsDbStore::~FsDbStore()
{
}

/**
 * create the container and its directory when either is missing, and bring an
 * existing container up to its full length.
 *
 * @return  pdi_err_t
 */
pdi_err_t FsDbStore::init()
{
    m_ready = false;

    pdiutil::string _dir = CHARPTR_WRAP(DB_STORE_DIR);
    pdiutil::string _file = CHARPTR_WRAP(DB_STORE_FILE);

    if (!__i_fs.isDirExist(_dir.c_str()))
    {
        if (__i_fs.createDirectory(_dir.c_str()) < 0)
        {
            return STORAGE_ERROR_NO_PARENT;
        }

        __i_fs.setFilePermissions(_dir.c_str(), DB_STORE_DIR_PERMS);
        __i_fs.setFileOwner(_dir.c_str(), 0, 0);
    }

    if (!__i_fs.isFileExist(_file.c_str()))
    {
        if (__i_fs.createFile(_file.c_str(), "", 0) < 0)
        {
            return PDI_ERR_IO;
        }

        __i_fs.setFilePermissions(_file.c_str(), DB_STORE_FILE_PERMS);
        __i_fs.setFileOwner(_file.c_str(), 0, 0);
    }

    // a container short of its full length is grown by writing its last byte,
    // the gap ahead of it is zero filled on the way
    if (__i_fs.getFileSize(_file.c_str()) < (int64_t)DB_CONTAINER_BYTES)
    {
        const char _pad = 0;

        if (__i_fs.editFile(_file.c_str(), DB_CONTAINER_BYTES - 1, &_pad, 1) < 0)
        {
            return PDI_ERR_NO_SPACE;
        }
    }

    m_ready = true;

    return PDI_OK;
}

/**
 * copy a byte range out of the container.
 *
 * @param   uint32_t  offset
 * @param   uint8_t*  buf
 * @param   uint32_t  len
 * @return  bool
 */
bool FsDbStore::read(uint32_t offset, uint8_t *buf, uint32_t len)
{
    if (!m_ready || offset + len > capacity())
    {
        return false;
    }

    if (0 == len)
    {
        return true;
    }

    pdiutil::string _file = CHARPTR_WRAP(DB_STORE_FILE);
    uint32_t _got = 0;

    __i_fs.readFile(_file.c_str(), DB_FS_IO_CHUNK_BYTES, [&](char *_data, uint32_t _size) -> bool {

        uint32_t _take = (_got + _size) > len ? (len - _got) : _size;

        memcpy(&buf[_got], _data, _take);
        _got += _take;

        return _got < len;
    }, offset);

    return _got == len;
}

/**
 * hand a byte range to fn through one open of the container, in its own chunks.
 *
 * @param   uint32_t  offset
 * @param   uint32_t  len
 * @param   function  fn
 * @return  bool
 */
bool FsDbStore::read_each(uint32_t offset, uint32_t len, uint8_t *, uint16_t,
                          pdiutil::function<void(const uint8_t *, uint32_t)> fn)
{
    if (!m_ready || offset + len > capacity())
    {
        return false;
    }

    if (0 == len)
    {
        return true;
    }

    pdiutil::string _file = CHARPTR_WRAP(DB_STORE_FILE);
    uint32_t _got = 0;

    __i_fs.readFile(_file.c_str(), DB_FS_IO_CHUNK_BYTES, [&](char *_data, uint32_t _size) -> bool {

        uint32_t _take = (_got + _size) > len ? (len - _got) : _size;

        fn((const uint8_t *)_data, _take);
        _got += _take;

        return _got < len;
    }, offset);

    return _got == len;
}

/**
 * write a byte range into the container, in place at its offset.
 *
 * @param   uint32_t        offset
 * @param   const uint8_t*  buf
 * @param   uint32_t        len
 * @return  bool
 */
bool FsDbStore::write(uint32_t offset, const uint8_t *buf, uint32_t len)
{
    if (!m_ready || offset + len > capacity())
    {
        return false;
    }

    if (0 == len)
    {
        return true;
    }

    pdiutil::string _file = CHARPTR_WRAP(DB_STORE_FILE);

    return __i_fs.editFile(_file.c_str(), offset, (const char *)buf, len) >= 0;
}

/**
 * writes reach the container as they are made, nothing is held back.
 *
 * @return  bool
 */
bool FsDbStore::flush()
{
    return m_ready;
}

/**
 * total bytes the container holds.
 *
 * @return  uint32_t
 */
uint32_t FsDbStore::capacity()
{
    return DB_CONTAINER_BYTES;
}

FsDbStore __i_fs_dbstore;

#endif
