/**************************** Database Layout Engine **************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 22nd Aug 2026
******************************************************************************/

#include "DbLayout.h"
#include <utility/Checksum.h>
#include <utility/DataTypeConversions.h>
#include <utility/SafeAlloc.h>

#ifdef ENABLE_DB_SEALING
#include <database/core/DbKey.h>
#include <utility/crypto/hmac/hmac_sha256.h>
#include <utility/crypto/symmetric/aes/aes.h>
#endif

static const uint8_t DB_MAGIC[3] = {'P', 'D', 'B'};

static void db_put_entry(uint8_t *_buf, const db_dir_entry_t &_entry)
{
    Uint16ToBytesLE(_entry.m_id, &_buf[0]);
    Uint16ToBytesLE(_entry.m_version, &_buf[2]);
    Uint16ToBytesLE(_entry.m_offset, &_buf[4]);
    Uint16ToBytesLE(_entry.m_cap, &_buf[6]);
    Uint16ToBytesLE(_entry.m_len, &_buf[8]);
    Uint16ToBytesLE(_entry.m_crc, &_buf[10]);
    _buf[12] = _entry.m_flags;
}

/**
 * whether a table's record is sealed in this build.
 */
static bool db_seals(const struct_tables &_table)
{
#ifdef ENABLE_DB_SEALING
    return _table.m_table_secret;
#else
    return false;
#endif
}

/**
 * bytes a table's record occupies, the seal it carries included.
 */
static uint16_t db_slot_for(const struct_tables &_table)
{
    return db_seals(_table) ? (uint16_t)(_table.m_table_size + DB_SEAL_OVERHEAD) : _table.m_table_size;
}

/**
 * bytes a record actually occupies on the medium. A sealed record carries its
 * nonce and tag on top of the payload length the caller sees.
 */
static uint16_t db_stored_bytes(const db_dir_entry_t &_entry)
{
    if (0 == _entry.m_len)
    {
        return 0;
    }

    return (_entry.m_flags & DB_ENTRY_FLAG_SEALED) ? (uint16_t)(_entry.m_len + DB_SEAL_OVERHEAD) : _entry.m_len;
}

/**
 * Constructor
 */
DbLayout::DbLayout() : m_store(nullptr), m_firmware(0), m_entries(0), m_year(0), m_mounted(false), m_formatted(false)
{
}

/**
 * Destructor
 */
DbLayout::~DbLayout()
{
}

/**
 * bind a store and reconcile it against the registered tables.
 *
 * @param   iDbStoreInterface*  _store
 * @param   registered table descriptors  _tables
 * @return  pdi_err_t
 */
pdi_err_t DbLayout::mount(iDbStoreInterface *_store, pdiutil::vector<struct_tables> &_tables)
{
    m_store = _store;
    m_mounted = false;
    m_formatted = false;

    if (nullptr == m_store)
    {
        return PDI_ERR_NULL_PTR;
    }

    if (DB_PAYLOAD_START > m_store->capacity())
    {
        return PDI_ERR_NO_SPACE;
    }

    if (PDI_OK != read_superblock())
    {
        pdi_err_t _err = format(_tables);
        if (PDI_OK != _err)
        {
            return _err;
        }

        m_mounted = true;
        m_formatted = true;
        return PDI_OK;
    }

    if (PDI_OK != read_directory())
    {
        return PDI_ERR_CORRUPT;
    }

    m_mounted = true;

    if (!geometry_matches(_tables))
    {
        return repack(_tables);
    }

    return PDI_OK;
}

/**
 * read and verify the superblock.
 *
 * @return  pdi_err_t
 */
pdi_err_t DbLayout::read_superblock()
{
    uint8_t _buf[DB_SUPERBLOCK_BYTES];

    if (!m_store->read(DB_REGION_START, _buf, DB_SUPERBLOCK_BYTES))
    {
        return PDI_ERR_IO;
    }

    if (_buf[0] != DB_MAGIC[0] || _buf[1] != DB_MAGIC[1] || _buf[2] != DB_MAGIC[2])
    {
        return PDI_ERR_CORRUPT;
    }

    if (_buf[3] != DB_FORMAT_VERSION)
    {
        return PDI_ERR_NOT_SUPPORTED;
    }

    if (BytesToUint16LE(&_buf[10]) != crc16Ccitt(_buf, DB_SUPERBLOCK_BYTES - 2))
    {
        return PDI_ERR_CRC;
    }

    m_firmware = BytesToUint32LE(&_buf[4]);
    m_year = _buf[8];
    m_entries = _buf[9];

    if (m_entries > MAX_TABLES)
    {
        return PDI_ERR_CORRUPT;
    }

    return PDI_OK;
}

/**
 * write the superblock back with a fresh crc.
 *
 * @return  pdi_err_t
 */
pdi_err_t DbLayout::write_superblock()
{
    uint8_t _buf[DB_SUPERBLOCK_BYTES];

    _buf[0] = DB_MAGIC[0];
    _buf[1] = DB_MAGIC[1];
    _buf[2] = DB_MAGIC[2];
    _buf[3] = DB_FORMAT_VERSION;
    Uint32ToBytesLE(m_firmware, &_buf[4]);
    _buf[8] = m_year;
    _buf[9] = m_entries;
    Uint16ToBytesLE(crc16Ccitt(_buf, DB_SUPERBLOCK_BYTES - 2), &_buf[10]);

    if (!m_store->write(DB_REGION_START, _buf, DB_SUPERBLOCK_BYTES))
    {
        return PDI_ERR_IO;
    }

    return m_store->flush() ? PDI_OK : PDI_ERR_IO;
}

/**
 * load every directory entry into ram.
 *
 * @return  pdi_err_t
 */
pdi_err_t DbLayout::read_directory()
{
    uint8_t _buf[DB_DIR_ENTRY_BYTES];

    for (uint8_t i = 0; i < m_entries; i++)
    {
        if (!m_store->read(DB_DIR_START + (i * DB_DIR_ENTRY_BYTES), _buf, DB_DIR_ENTRY_BYTES))
        {
            return PDI_ERR_IO;
        }

        m_directory[i].m_id = BytesToUint16LE(&_buf[0]);
        m_directory[i].m_version = BytesToUint16LE(&_buf[2]);
        m_directory[i].m_offset = BytesToUint16LE(&_buf[4]);
        m_directory[i].m_cap = BytesToUint16LE(&_buf[6]);
        m_directory[i].m_len = BytesToUint16LE(&_buf[8]);
        m_directory[i].m_crc = BytesToUint16LE(&_buf[10]);
        m_directory[i].m_flags = _buf[12];

        if ((uint32_t)m_directory[i].m_offset + m_directory[i].m_cap > m_store->capacity() ||
            m_directory[i].m_len > m_directory[i].m_cap)
        {
            return PDI_ERR_CORRUPT;
        }
    }

    return PDI_OK;
}

/**
 * write one directory entry back to the store.
 *
 * @param   uint8_t  _index
 * @return  pdi_err_t
 */
pdi_err_t DbLayout::write_entry(uint8_t _index)
{
    uint8_t _buf[DB_DIR_ENTRY_BYTES];

    db_put_entry(_buf, m_directory[_index]);

    if (!m_store->write(DB_DIR_START + (_index * DB_DIR_ENTRY_BYTES), _buf, DB_DIR_ENTRY_BYTES))
    {
        return PDI_ERR_IO;
    }

    return PDI_OK;
}

/**
 * whether the stored geometry still matches every registered table.
 *
 * @param   registered table descriptors  _tables
 * @return  bool
 */
bool DbLayout::geometry_matches(pdiutil::vector<struct_tables> &_tables)
{
    if (_tables.size() != m_entries)
    {
        return false;
    }

    for (uint8_t i = 0; i < _tables.size(); i++)
    {
        db_dir_entry_t *_entry = find_entry(_tables[i].m_table_id);

        if (nullptr == _entry || _entry->m_cap != db_slot_for(_tables[i]))
        {
            return false;
        }
    }

    return true;
}

/**
 * find a directory entry by table id.
 *
 * @param   uint16_t  _id
 * @return  db_dir_entry_t*  entry or nullptr
 */
db_dir_entry_t *DbLayout::find_entry(uint16_t _id)
{
    for (uint8_t i = 0; i < m_entries; i++)
    {
        if (m_directory[i].m_id == _id)
        {
            return &m_directory[i];
        }
    }

    return nullptr;
}

/**
 * lay down a fresh superblock and directory, every record empty.
 *
 * @param   registered table descriptors  _tables
 * @return  pdi_err_t
 */
pdi_err_t DbLayout::format(pdiutil::vector<struct_tables> &_tables)
{
    uint32_t _offset = DB_PAYLOAD_START;

    m_entries = 0;

    for (uint8_t i = 0; i < _tables.size() && m_entries < MAX_TABLES; i++)
    {
        uint16_t _cap = db_slot_for(_tables[i]);

        if (_offset + _cap > m_store->capacity())
        {
            return PDI_ERR_NO_SPACE;
        }

        m_directory[m_entries].m_id = _tables[i].m_table_id;
        m_directory[m_entries].m_version = _tables[i].m_table_version;
        m_directory[m_entries].m_offset = (uint16_t)_offset;
        m_directory[m_entries].m_cap = _cap;
        m_directory[m_entries].m_len = 0;
        m_directory[m_entries].m_crc = 0;
        m_directory[m_entries].m_flags = db_seals(_tables[i]) ? DB_ENTRY_FLAG_SEALED : 0;

        _offset += _cap;
        m_entries++;
    }

    for (uint8_t i = 0; i < m_entries; i++)
    {
        pdi_err_t _err = write_entry(i);

        if (PDI_OK != _err)
        {
            return _err;
        }
    }

    return write_superblock();
}

/**
 * rebuild the directory for the registered tables, carrying payloads by id.
 *
 * The new layout is decided first, then only the records that actually change
 * offset are lifted through a buffer. A table appended at the end, or the last
 * one growing, moves nothing and so asks for no memory at all.
 *
 * A record whose stored bytes no longer fit its slot returns to its defaults,
 * which is what a struct that shrank has to mean.
 *
 * @param   registered table descriptors  _tables
 * @return  pdi_err_t
 */
pdi_err_t DbLayout::repack(pdiutil::vector<struct_tables> &_tables)
{
    db_dir_entry_t _old[MAX_TABLES];
    uint16_t _from[MAX_TABLES];
    uint8_t _old_entries = m_entries;

    for (uint8_t i = 0; i < m_entries; i++)
    {
        _old[i] = m_directory[i];
    }

    uint32_t _offset = DB_PAYLOAD_START;
    m_entries = 0;

    for (uint8_t i = 0; i < _tables.size() && m_entries < MAX_TABLES; i++)
    {
        uint16_t _cap = db_slot_for(_tables[i]);

        if (_offset + _cap > m_store->capacity())
        {
            return PDI_ERR_NO_SPACE;
        }

        db_dir_entry_t &_entry = m_directory[m_entries];

        _entry.m_id = _tables[i].m_table_id;
        _entry.m_offset = (uint16_t)_offset;
        _entry.m_cap = _cap;
        _entry.m_version = _tables[i].m_table_version;
        _entry.m_len = 0;
        _entry.m_crc = 0;
        _entry.m_flags = db_seals(_tables[i]) ? DB_ENTRY_FLAG_SEALED : 0;

        _from[m_entries] = 0;

        for (uint8_t j = 0; j < _old_entries; j++)
        {
            if (_old[j].m_id != _entry.m_id)
            {
                continue;
            }

            uint16_t _stored = db_stored_bytes(_old[j]);
            bool _same_kind = (_old[j].m_flags & DB_ENTRY_FLAG_SEALED) == (_entry.m_flags & DB_ENTRY_FLAG_SEALED);

            if (0 != _stored && _same_kind && _stored <= _cap)
            {
                _entry.m_len = _old[j].m_len;
                _entry.m_crc = _old[j].m_crc;
                _entry.m_version = _old[j].m_version;
                _from[m_entries] = _old[j].m_offset;
            }

            break;
        }

        _offset += _cap;
        m_entries++;
    }

    // a record already sitting at its new offset occupies exactly the range it
    // keeps, and no other record is written into that range, so it stays put
    uint32_t _carried = 0;

    for (uint8_t i = 0; i < m_entries; i++)
    {
        if (0 != _from[i] && _from[i] != m_directory[i].m_offset)
        {
            _carried += db_stored_bytes(m_directory[i]);
        }
    }

    if (0 != _carried)
    {
        uint8_t *_scratch = pdiutil::safe_new_array<uint8_t>(_carried);

        if (nullptr == _scratch)
        {
            return PDI_ERR_NO_MEM;
        }

        uint32_t _at = 0;

        for (uint8_t i = 0; i < m_entries; i++)
        {
            if (0 != _from[i] && _from[i] != m_directory[i].m_offset)
            {
                uint16_t _stored = db_stored_bytes(m_directory[i]);
                m_store->read(_from[i], &_scratch[_at], _stored);
                _at += _stored;
            }
        }

        _at = 0;

        for (uint8_t i = 0; i < m_entries; i++)
        {
            if (0 != _from[i] && _from[i] != m_directory[i].m_offset)
            {
                uint16_t _stored = db_stored_bytes(m_directory[i]);
                m_store->write(m_directory[i].m_offset, &_scratch[_at], _stored);
                _at += _stored;
            }
        }

        pdiutil::safe_delete_array(_scratch);
    }

    for (uint8_t i = 0; i < m_entries; i++)
    {
        pdi_err_t _err = write_entry(i);

        if (PDI_OK != _err)
        {
            return _err;
        }
    }

    return write_superblock();
}

/**
 * run a plain record past its checksum without copying any of it out.
 *
 * @param   db_dir_entry_t* _entry
 * @return  pdi_err_t
 */
pdi_err_t DbLayout::verify_plain(db_dir_entry_t *_entry)
{
    uint8_t _window[DB_CRC_WINDOW_BYTES];
    uint16_t _crc = 0xFFFF;

    if (!m_store->read_each(_entry->m_offset, _entry->m_len, _window, sizeof(_window),
                            [&](const uint8_t *_piece, uint32_t _size) { _crc = crc16CcittUpdate(_crc, _piece, _size); }))
    {
        return PDI_ERR_IO;
    }

    return _crc == _entry->m_crc ? PDI_OK : PDI_ERR_CRC;
}

#ifdef ENABLE_DB_SEALING
/**
 * The cipher and mac states a seal works through, kept together so one
 * allocation carries a whole operation.
 *
 * These are by far the largest objects the database handles and they do not
 * belong on the stack a write arrives on. A seal reached through the web server
 * already sits below lwip, the http server and a page handler, and on a device
 * whose loop runs on a small fixed stack the cipher state alone is a sizeable
 * part of what is left. Every device that declares the sealing capability has a
 * heap, so the states are taken from there for as long as the operation runs
 * and given back as soon as it ends.
 */
struct db_seal_scratch_t
{
    AES_ctx m_aes;
    hmac_sha256_context m_mac;
    uint8_t m_window[DB_SEAL_CHUNK_BYTES];
    uint8_t m_tag[32];
    uint8_t m_iv[DB_SEAL_IV_BYTES];
};

/**
 * seal a payload into its slot as nonce, ciphertext then tag.
 *
 * The payload is taken a block at a time so a record is never held in memory
 * whole, and the tag is taken over the nonce and the ciphertext, so a tampered
 * record fails before anything is decrypted.
 *
 * @param   db_dir_entry_t* _entry
 * @param   const uint8_t*  _buf
 * @param   uint16_t        _len
 * @return  pdi_err_t
 */
pdi_err_t DbLayout::seal_record(db_dir_entry_t *_entry, const uint8_t *_buf, uint16_t _len)
{
    if (!__db_key.is_ready())
    {
        return PDI_ERR_NOT_INITIALIZED;
    }

    db_seal_scratch_t *_scratch = pdiutil::safe_new<db_seal_scratch_t>();

    if (nullptr == _scratch)
    {
        return PDI_ERR_NO_MEM;
    }

    for (uint8_t i = 0; i < DB_SEAL_IV_BYTES; i += 4)
    {
        uint32_t _r = __i_dvc_ctrl.random_now();

        for (uint8_t b = 0; b < 4 && (i + b) < DB_SEAL_IV_BYTES; b++)
        {
            _scratch->m_iv[i + b] = (uint8_t)((_r >> (b * 8)) & 0xFF);
        }
    }

    pdi_err_t _err = PDI_OK;

    if (!m_store->write(_entry->m_offset, _scratch->m_iv, DB_SEAL_IV_BYTES))
    {
        _err = PDI_ERR_IO;
    }

    if (PDI_OK == _err)
    {
        AES_init_ctx_iv(&_scratch->m_aes, __db_key.key(), _scratch->m_iv);

        hmac_sha256_init(&_scratch->m_mac, __db_key.key(), DB_KEY_BYTES);
        hmac_sha256_update(&_scratch->m_mac, _scratch->m_iv, DB_SEAL_IV_BYTES);

        uint16_t _at = 0;

        while (_at < _len)
        {
            uint16_t _chunk = _len - _at;

            if (_chunk > DB_SEAL_CHUNK_BYTES)
            {
                _chunk = DB_SEAL_CHUNK_BYTES;
            }

            memcpy(_scratch->m_window, &_buf[_at], _chunk);
            AES_CTR_xcrypt_buffer(&_scratch->m_aes, _scratch->m_window, _chunk);

            if (!m_store->write(_entry->m_offset + DB_SEAL_IV_BYTES + _at, _scratch->m_window, _chunk))
            {
                _err = PDI_ERR_IO;
                break;
            }

            hmac_sha256_update(&_scratch->m_mac, _scratch->m_window, _chunk);
            _at += _chunk;
        }
    }

    if (PDI_OK == _err)
    {
        hmac_sha256_final(&_scratch->m_mac, _scratch->m_tag);

        if (!m_store->write(_entry->m_offset + DB_SEAL_IV_BYTES + _len, _scratch->m_tag, DB_SEAL_TAG_BYTES))
        {
            _err = PDI_ERR_IO;
        }
    }

    pdiutil::safe_delete(_scratch);

    if (PDI_OK != _err)
    {
        return _err;
    }

    _entry->m_len = _len;
    _entry->m_crc = 0;

    return PDI_OK;
}

/**
 * recompute a sealed record's tag and compare it with _expected.
 *
 * The ciphertext is run through in passes, so verifying a record never needs
 * room to hold it.
 *
 * @param   db_dir_entry_t* _entry
 * @param   uint8_t*        _expected
 * @return  pdi_err_t
 */
pdi_err_t DbLayout::verify_sealed(db_dir_entry_t *_entry, const uint8_t *_expected, db_seal_scratch_t *_scratch)
{
    if (!__db_key.is_ready())
    {
        return PDI_ERR_NOT_INITIALIZED;
    }

    if (nullptr == _scratch)
    {
        return PDI_ERR_NO_MEM;
    }

    hmac_sha256_init(&_scratch->m_mac, __db_key.key(), DB_KEY_BYTES);

    if (!m_store->read_each(_entry->m_offset, DB_SEAL_IV_BYTES + _entry->m_len, _scratch->m_window, sizeof(_scratch->m_window),
                            [&](const uint8_t *_piece, uint32_t _size) { hmac_sha256_update(&_scratch->m_mac, _piece, _size); }))
    {
        return PDI_ERR_IO;
    }

    hmac_sha256_final(&_scratch->m_mac, _scratch->m_tag);

    return 0 == memcmp(_scratch->m_tag, _expected, DB_SEAL_TAG_BYTES) ? PDI_OK : PDI_ERR_CRC;
}

/**
 * check a sealed record's tag and lay its payload over _buf.
 *
 * @param   db_dir_entry_t* _entry
 * @param   uint8_t*        _buf
 * @param   uint16_t        _read
 * @return  pdi_err_t
 */
pdi_err_t DbLayout::unseal_record(db_dir_entry_t *_entry, uint8_t *_buf, uint16_t _read)
{
    // the stored tag is the one thing verify_sealed must not be able to write
    // over, so it stays out of the scratch it works in.
    uint8_t _stored_tag[DB_SEAL_TAG_BYTES];

    if (!m_store->read(_entry->m_offset + DB_SEAL_IV_BYTES + _entry->m_len, _stored_tag, DB_SEAL_TAG_BYTES))
    {
        return PDI_ERR_IO;
    }

    db_seal_scratch_t *_scratch = pdiutil::safe_new<db_seal_scratch_t>();

    if (nullptr == _scratch)
    {
        return PDI_ERR_NO_MEM;
    }

    pdi_err_t _err = verify_sealed(_entry, _stored_tag, _scratch);

    if (PDI_OK == _err)
    {
        if (m_store->read(_entry->m_offset, _scratch->m_iv, DB_SEAL_IV_BYTES) &&
            m_store->read(_entry->m_offset + DB_SEAL_IV_BYTES, _buf, _read))
        {
            AES_init_ctx_iv(&_scratch->m_aes, __db_key.key(), _scratch->m_iv);
            AES_CTR_xcrypt_buffer(&_scratch->m_aes, _buf, _read);
        }
        else
        {
            _err = PDI_ERR_IO;
        }
    }

    pdiutil::safe_delete(_scratch);

    return _err;
}
#endif

#ifdef ENABLE_STORAGE_SERVICE
/**
 * copy every record this layout knows about out of _source.
 *
 * Records move one at a time through a buffer the size of the largest of them,
 * so carrying a tier across never asks for the whole database at once.
 *
 * @param   DbLayout&  _source
 * @return  pdi_err_t
 */
pdi_err_t DbLayout::copy_from(DbLayout &_source)
{
    if (!m_mounted || !_source.is_mounted())
    {
        return PDI_ERR_NOT_INITIALIZED;
    }

    uint16_t _widest = 0;

    for (uint8_t i = 0; i < m_entries; i++)
    {
        uint16_t _payload = m_directory[i].m_cap;

        if ((m_directory[i].m_flags & DB_ENTRY_FLAG_SEALED) && _payload >= DB_SEAL_OVERHEAD)
        {
            _payload -= DB_SEAL_OVERHEAD;
        }

        if (_payload > _widest)
        {
            _widest = _payload;
        }
    }

    if (0 == _widest)
    {
        return PDI_OK;
    }

    uint8_t *_buf = pdiutil::safe_new_array<uint8_t>(_widest);

    if (nullptr == _buf)
    {
        return PDI_ERR_NO_MEM;
    }

    pdi_err_t _status = PDI_OK;

    for (uint8_t i = 0; i < m_entries; i++)
    {
        uint16_t _id = m_directory[i].m_id;
        uint16_t _len = 0;
        uint16_t _version = 0;
        pdi_err_t _err = _source.read_record(_id, _buf, _widest, _len, _version);

        if (PDI_OK != _err || 0 == _len)
        {
            if (PDI_OK != clear_record(_id))
            {
                _status = PDI_ERR_IO;
            }

            continue;
        }

        if (PDI_OK != write_record(_id, _buf, _len, _version))
        {
            _status = PDI_ERR_IO;
        }
    }

    memset(_buf, 0, _widest);
    pdiutil::safe_delete_array(_buf);

    return _status;
}
#endif

/**
 * run a record past its checksum or tag without copying any of it out.
 *
 * @param   uint16_t  _id
 * @return  pdi_err_t
 */
pdi_err_t DbLayout::verify_record(uint16_t _id)
{
    if (!m_mounted)
    {
        return PDI_ERR_NOT_INITIALIZED;
    }

    db_dir_entry_t *_entry = find_entry(_id);

    if (nullptr == _entry)
    {
        return PDI_ERR_NOT_FOUND;
    }

    if (0 == _entry->m_len)
    {
        return PDI_OK;
    }

#ifdef ENABLE_DB_SEALING
    if (_entry->m_flags & DB_ENTRY_FLAG_SEALED)
    {
        uint8_t _stored_tag[DB_SEAL_TAG_BYTES];

        if (!m_store->read(_entry->m_offset + DB_SEAL_IV_BYTES + _entry->m_len, _stored_tag, DB_SEAL_TAG_BYTES))
        {
            return PDI_ERR_IO;
        }

        db_seal_scratch_t *_scratch = pdiutil::safe_new<db_seal_scratch_t>();

        if (nullptr == _scratch)
        {
            return PDI_ERR_NO_MEM;
        }

        pdi_err_t _err = verify_sealed(_entry, _stored_tag, _scratch);

        pdiutil::safe_delete(_scratch);

        return _err;
    }
#endif

    return verify_plain(_entry);
}

/**
 * copy a record's stored bytes over _buf.
 *
 * @param   uint16_t  _id
 * @param   uint8_t*  _buf
 * @param   uint16_t  _cap
 * @param   uint16_t& _len
 * @param   uint16_t& _version
 * @return  pdi_err_t
 */
pdi_err_t DbLayout::read_record(uint16_t _id, uint8_t *_buf, uint16_t _cap, uint16_t &_len, uint16_t &_version)
{
    _len = 0;
    _version = 0;

    if (!m_mounted)
    {
        return PDI_ERR_NOT_INITIALIZED;
    }

    db_dir_entry_t *_entry = find_entry(_id);

    if (nullptr == _entry)
    {
        return PDI_ERR_NOT_FOUND;
    }

    if (0 == _entry->m_len)
    {
        _version = _entry->m_version;
        return PDI_OK;
    }

    uint16_t _read = _entry->m_len > _cap ? _cap : _entry->m_len;

#ifdef ENABLE_DB_SEALING
    if (_entry->m_flags & DB_ENTRY_FLAG_SEALED)
    {
        pdi_err_t _err = unseal_record(_entry, _buf, _read);

        if (PDI_OK != _err)
        {
            return _err;
        }

        _len = _read;
        _version = _entry->m_version;

        return PDI_OK;
    }
#endif

    pdi_err_t _err = verify_plain(_entry);

    if (PDI_OK != _err)
    {
        return _err;
    }

    if (!m_store->read(_entry->m_offset, _buf, _read))
    {
        return PDI_ERR_IO;
    }

    _len = _read;
    _version = _entry->m_version;

    return PDI_OK;
}

/**
 * store _len bytes as the record for _id.
 *
 * @param   uint16_t        _id
 * @param   const uint8_t*  _buf
 * @param   uint16_t        _len
 * @param   uint16_t        _version
 * @return  pdi_err_t
 */
pdi_err_t DbLayout::write_record(uint16_t _id, const uint8_t *_buf, uint16_t _len, uint16_t _version)
{
    if (!m_mounted)
    {
        return PDI_ERR_NOT_INITIALIZED;
    }

    db_dir_entry_t *_entry = find_entry(_id);

    if (nullptr == _entry)
    {
        return PDI_ERR_NOT_FOUND;
    }

    bool _sealed = 0 != (_entry->m_flags & DB_ENTRY_FLAG_SEALED);

    if ((uint32_t)_len + (_sealed ? DB_SEAL_OVERHEAD : 0) > _entry->m_cap)
    {
        return PDI_ERR_NO_SPACE;
    }

#ifdef ENABLE_DB_SEALING
    if (_sealed)
    {
        pdi_err_t _err = seal_record(_entry, _buf, _len);

        if (PDI_OK != _err)
        {
            return _err;
        }
    }
    else
#endif
    {
        if (!m_store->write(_entry->m_offset, _buf, _len))
        {
            return PDI_ERR_IO;
        }

        _entry->m_len = _len;
        _entry->m_crc = crc16Ccitt(_buf, _len);
    }

    _entry->m_version = _version;

    pdi_err_t _err = write_entry((uint8_t)(_entry - &m_directory[0]));

    if (PDI_OK != _err)
    {
        return _err;
    }

    return m_store->flush() ? PDI_OK : PDI_ERR_IO;
}

/**
 * drop a record's contents so reads fall back to struct defaults.
 *
 * @param   uint16_t  _id
 * @return  pdi_err_t
 */
pdi_err_t DbLayout::clear_record(uint16_t _id)
{
    if (!m_mounted)
    {
        return PDI_ERR_NOT_INITIALIZED;
    }

    db_dir_entry_t *_entry = find_entry(_id);

    if (nullptr == _entry)
    {
        return PDI_ERR_NOT_FOUND;
    }

    _entry->m_len = 0;
    _entry->m_crc = 0;

    pdi_err_t _err = write_entry((uint8_t)(_entry - &m_directory[0]));

    if (PDI_OK != _err)
    {
        return _err;
    }

    return m_store->flush() ? PDI_OK : PDI_ERR_IO;
}

/**
 * record the firmware version carried in the superblock.
 *
 * @param   uint32_t  _version
 */
void DbLayout::set_firmware_version(uint32_t _version)
{
    if (m_mounted && m_firmware != _version)
    {
        m_firmware = _version;
        write_superblock();
    }
}

/**
 * record the launch year carried in the superblock.
 *
 * @param   uint8_t  _year
 */
void DbLayout::set_launch_year(uint8_t _year)
{
    if (m_mounted && m_year != _year)
    {
        m_year = _year;
        write_superblock();
    }
}

/**
 * payload bytes reserved by all records.
 *
 * @return  uint16_t
 */
uint16_t DbLayout::used_bytes() const
{
    uint16_t _used = 0;

    for (uint8_t i = 0; i < m_entries; i++)
    {
        _used += m_directory[i].m_cap;
    }

    return _used;
}

/**
 * payload bytes still free in the store.
 *
 * @return  uint16_t
 */
uint16_t DbLayout::free_bytes() const
{
    if (nullptr == m_store)
    {
        return 0;
    }

    uint32_t _payload_room = m_store->capacity() - DB_PAYLOAD_START;

    return (uint16_t)(_payload_room - used_bytes());
}

/**
 * directory entry by index.
 *
 * @param   uint8_t  _index
 * @return  const db_dir_entry_t*  entry or nullptr
 */
const db_dir_entry_t *DbLayout::entry_at(uint8_t _index) const
{
    return _index < m_entries ? &m_directory[_index] : nullptr;
}

DbLayout __db_layout;
