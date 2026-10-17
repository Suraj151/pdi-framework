/*************************** Database Layout Tests ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 22nd Aug 2026
******************************************************************************/

#include <pditest.h>
#include <database/core/DbLayout.h>
#include <database/core/DbKey.h>
#include <utility/Checksum.h>
#include <utility/crypto/hmac/hmac_sha256.h>

/**
 * A store held entirely in ram, so a test can look at the bytes the engine
 * wrote and corrupt them the way a failing flash sector would.
 */
struct FakeDbStore : public iDbStoreInterface
{
    static const uint32_t SIZE = 4096;

    uint8_t m_bytes[SIZE];
    uint32_t m_flushes;

    FakeDbStore() : m_flushes(0)
    {
        memset(m_bytes, 0xFF, SIZE);
    }

    bool read(uint32_t offset, uint8_t *buf, uint32_t len) override
    {
        if (offset + len > SIZE) return false;
        memcpy(buf, &m_bytes[offset], len);
        return true;
    }

    bool write(uint32_t offset, const uint8_t *buf, uint32_t len) override
    {
        if (offset + len > SIZE) return false;
        memcpy(&m_bytes[offset], buf, len);
        return true;
    }

    bool flush() override
    {
        m_flushes++;
        return true;
    }

    uint32_t capacity() override { return SIZE; }
};

static struct_tables makeTable(uint16_t id, uint16_t size, uint16_t version = 1, bool secret = false)
{
    struct_tables table;
    memset(&table, 0, sizeof(table));
    table.m_table_id = id;
    table.m_table_size = size;
    table.m_table_version = version;
    table.m_table_secret = secret;
    return table;
}

/**
 * A sealed table beside a plain one. The key comes from the eeprom the mock
 * device keeps, the same place a real board holds it.
 */
static pdiutil::vector<struct_tables> sealedTables(uint16_t secretSize = 64)
{
    __i_db.beginConfigs(__i_db.getMaxDBSize());
    __db_key.load();

    pdiutil::vector<struct_tables> tables;
    tables.push_back(makeTable(1, secretSize, 1, true));
    tables.push_back(makeTable(2, 32, 1, false));
    return tables;
}

static pdiutil::vector<struct_tables> twoTables(uint16_t firstSize = 16, uint16_t secondSize = 32)
{
    pdiutil::vector<struct_tables> tables;
    tables.push_back(makeTable(1, firstSize));
    tables.push_back(makeTable(2, secondSize));
    return tables;
}

TEST(dblayout, mounting_a_blank_store_lays_down_a_fresh_database)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = twoTables();

    ASSERT_EQ(layout.mount(&store, tables), PDI_OK);
    ASSERT_TRUE(layout.is_mounted());
    ASSERT_TRUE(layout.was_formatted());
    ASSERT_EQ(layout.entry_count(), (uint8_t)2);
}

TEST(dblayout, a_fresh_database_reports_every_record_empty)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = twoTables();
    layout.mount(&store, tables);

    uint8_t buf[16];
    uint16_t len = 99;
    uint16_t version = 99;

    ASSERT_EQ(layout.read_record(1, buf, sizeof(buf), len, version), PDI_OK);
    ASSERT_EQ(len, (uint16_t)0);
}

TEST(dblayout, records_are_packed_and_do_not_share_bytes)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = twoTables(16, 32);
    layout.mount(&store, tables);

    const db_dir_entry_t *first = layout.entry_at(0);
    const db_dir_entry_t *second = layout.entry_at(1);

    ASSERT_NOT_NULL(first);
    ASSERT_NOT_NULL(second);
    ASSERT_EQ(first->m_offset, (uint16_t)DB_PAYLOAD_START);
    ASSERT_EQ(second->m_offset, (uint16_t)(DB_PAYLOAD_START + 16));
    ASSERT_EQ(first->m_cap, (uint16_t)16);
}

TEST(dblayout, a_written_record_reads_back_byte_for_byte)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = twoTables();
    layout.mount(&store, tables);

    uint8_t written[16];
    for (uint8_t i = 0; i < sizeof(written); i++) written[i] = (uint8_t)(i + 1);

    ASSERT_EQ(layout.write_record(1, written, sizeof(written), 1), PDI_OK);

    uint8_t readBack[16];
    memset(readBack, 0, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(layout.read_record(1, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_EQ(len, (uint16_t)16);
    ASSERT_EQ(version, (uint16_t)1);
    ASSERT_MEMEQ(readBack, written, sizeof(written));
}

TEST(dblayout, writing_one_record_leaves_its_neighbour_alone)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = twoTables();
    layout.mount(&store, tables);

    uint8_t second[32];
    memset(second, 0xAB, sizeof(second));
    layout.write_record(2, second, sizeof(second), 1);

    uint8_t first[16];
    memset(first, 0x11, sizeof(first));
    layout.write_record(1, first, sizeof(first), 1);

    uint8_t readBack[32];
    memset(readBack, 0, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(layout.read_record(2, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_MEMEQ(readBack, second, sizeof(second));
}

TEST(dblayout, an_unknown_id_is_not_found)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = twoTables();
    layout.mount(&store, tables);

    uint8_t buf[16];
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(layout.read_record(77, buf, sizeof(buf), len, version), PDI_ERR_NOT_FOUND);
}

TEST(dblayout, a_record_larger_than_its_slot_is_refused)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = twoTables();
    layout.mount(&store, tables);

    uint8_t oversized[64];
    memset(oversized, 0, sizeof(oversized));

    ASSERT_EQ(layout.write_record(1, oversized, sizeof(oversized), 1), PDI_ERR_NO_SPACE);
}

/**
 * Remounting is what a reboot does, so a record has to survive the engine
 * being torn down and brought back over the same bytes.
 */
TEST(dblayout, records_survive_a_remount)
{
    FakeDbStore store;
    pdiutil::vector<struct_tables> tables = twoTables();

    uint8_t written[16];
    memset(written, 0x5A, sizeof(written));

    {
        DbLayout layout;
        layout.mount(&store, tables);
        layout.write_record(1, written, sizeof(written), 1);
    }

    DbLayout remounted;
    ASSERT_EQ(remounted.mount(&store, tables), PDI_OK);
    ASSERT_FALSE(remounted.was_formatted());

    uint8_t readBack[16];
    memset(readBack, 0, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(remounted.read_record(1, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_MEMEQ(readBack, written, sizeof(written));
}

/**
 * The growth rule: a struct that gained fields still loads what was stored,
 * and reports the shorter length so the caller keeps its defaults for the rest.
 */
TEST(dblayout, a_grown_struct_reads_back_only_what_was_stored)
{
    FakeDbStore store;
    pdiutil::vector<struct_tables> before = twoTables(16, 32);

    uint8_t written[16];
    for (uint8_t i = 0; i < sizeof(written); i++) written[i] = (uint8_t)(i + 1);

    {
        DbLayout layout;
        layout.mount(&store, before);
        layout.write_record(1, written, sizeof(written), 1);
    }

    pdiutil::vector<struct_tables> after = twoTables(24, 32);
    DbLayout grown;
    ASSERT_EQ(grown.mount(&store, after), PDI_OK);

    uint8_t readBack[24];
    memset(readBack, 0xEE, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(grown.read_record(1, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_EQ(len, (uint16_t)16);
    ASSERT_MEMEQ(readBack, written, 16);
    ASSERT_EQ(readBack[16], (uint8_t)0xEE);
    ASSERT_EQ(readBack[23], (uint8_t)0xEE);
}

/**
 * A struct growing moves every record after it, and the repack has to carry
 * those payloads across rather than leaving them at their old offsets.
 */
TEST(dblayout, a_repack_carries_a_later_record_to_its_new_home)
{
    FakeDbStore store;
    pdiutil::vector<struct_tables> before = twoTables(16, 32);

    uint8_t second[32];
    memset(second, 0xC3, sizeof(second));

    {
        DbLayout layout;
        layout.mount(&store, before);
        layout.write_record(2, second, sizeof(second), 1);
    }

    pdiutil::vector<struct_tables> after = twoTables(48, 32);
    DbLayout grown;
    ASSERT_EQ(grown.mount(&store, after), PDI_OK);

    const db_dir_entry_t *moved = grown.entry_at(1);
    ASSERT_NOT_NULL(moved);
    ASSERT_EQ(moved->m_offset, (uint16_t)(DB_PAYLOAD_START + 48));

    uint8_t readBack[32];
    memset(readBack, 0, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(grown.read_record(2, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_EQ(len, (uint16_t)32);
    ASSERT_MEMEQ(readBack, second, sizeof(second));
}

/**
 * A struct that shrank no longer describes its stored bytes, so the record
 * returns to its defaults rather than handing back a cut down payload.
 */
TEST(dblayout, a_shrunk_struct_returns_to_its_defaults)
{
    FakeDbStore store;
    pdiutil::vector<struct_tables> before = twoTables(32, 32);

    uint8_t written[32];
    memset(written, 0x6D, sizeof(written));

    {
        DbLayout layout;
        layout.mount(&store, before);
        layout.write_record(1, written, sizeof(written), 1);
    }

    pdiutil::vector<struct_tables> after = twoTables(16, 32);
    DbLayout shrunk;
    ASSERT_EQ(shrunk.mount(&store, after), PDI_OK);

    uint8_t readBack[16];
    memset(readBack, 0xC4, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(shrunk.read_record(1, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_EQ(len, (uint16_t)0);
    ASSERT_EQ(readBack[0], (uint8_t)0xC4);
}

/**
 * A record already sitting at its new offset is left where it is, which is what
 * keeps a repack from asking for memory it may not get on a small device.
 */
TEST(dblayout, appending_a_table_moves_no_existing_record)
{
    FakeDbStore store;
    pdiutil::vector<struct_tables> before = twoTables(16, 32);

    uint8_t first[16];
    uint8_t second[32];
    memset(first, 0x21, sizeof(first));
    memset(second, 0x43, sizeof(second));

    {
        DbLayout layout;
        layout.mount(&store, before);
        layout.write_record(1, first, sizeof(first), 1);
        layout.write_record(2, second, sizeof(second), 1);
    }

    pdiutil::vector<struct_tables> after = twoTables(16, 32);
    after.push_back(makeTable(3, 24));

    DbLayout grown;
    ASSERT_EQ(grown.mount(&store, after), PDI_OK);

    ASSERT_EQ(grown.entry_at(0)->m_offset, (uint16_t)DB_PAYLOAD_START);
    ASSERT_EQ(grown.entry_at(1)->m_offset, (uint16_t)(DB_PAYLOAD_START + 16));

    uint8_t readBack[32];
    uint16_t len = 0;
    uint16_t version = 0;

    memset(readBack, 0, sizeof(readBack));
    ASSERT_EQ(grown.read_record(1, readBack, 16, len, version), PDI_OK);
    ASSERT_MEMEQ(readBack, first, sizeof(first));

    memset(readBack, 0, sizeof(readBack));
    ASSERT_EQ(grown.read_record(2, readBack, 32, len, version), PDI_OK);
    ASSERT_MEMEQ(readBack, second, sizeof(second));
}

TEST(dblayout, verify_record_passes_a_good_record_and_fails_a_rotted_one)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = twoTables();
    layout.mount(&store, tables);

    uint8_t written[16];
    memset(written, 0x18, sizeof(written));
    layout.write_record(1, written, sizeof(written), 1);

    ASSERT_EQ(layout.verify_record(1), PDI_OK);
    ASSERT_EQ(layout.verify_record(2), PDI_OK);

    store.m_bytes[DB_PAYLOAD_START + 2] ^= 0xFF;
    ASSERT_EQ(layout.verify_record(1), PDI_ERR_CRC);
}

TEST(dblayout, a_table_added_later_joins_without_disturbing_the_others)
{
    FakeDbStore store;
    pdiutil::vector<struct_tables> before = twoTables();

    uint8_t written[16];
    memset(written, 0x77, sizeof(written));

    {
        DbLayout layout;
        layout.mount(&store, before);
        layout.write_record(1, written, sizeof(written), 1);
    }

    pdiutil::vector<struct_tables> after = twoTables();
    after.push_back(makeTable(3, 20));

    DbLayout grown;
    ASSERT_EQ(grown.mount(&store, after), PDI_OK);
    ASSERT_EQ(grown.entry_count(), (uint8_t)3);

    uint8_t readBack[16];
    memset(readBack, 0, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(grown.read_record(1, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_MEMEQ(readBack, written, sizeof(written));

    ASSERT_EQ(grown.read_record(3, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_EQ(len, (uint16_t)0);
}

TEST(dblayout, the_version_a_record_was_written_by_is_reported_back)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = twoTables();
    layout.mount(&store, tables);

    uint8_t written[16];
    memset(written, 0x01, sizeof(written));
    layout.write_record(1, written, sizeof(written), 4);

    uint8_t readBack[16];
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(layout.read_record(1, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_EQ(version, (uint16_t)4);
}

/**
 * A record whose bytes rotted has to be refused, and refused before it lands
 * on the caller's buffer, which is holding the struct defaults.
 */
TEST(dblayout, a_corrupt_record_is_refused_without_touching_the_caller)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = twoTables();
    layout.mount(&store, tables);

    uint8_t written[16];
    memset(written, 0x42, sizeof(written));
    layout.write_record(1, written, sizeof(written), 1);

    store.m_bytes[DB_PAYLOAD_START + 3] ^= 0xFF;

    uint8_t readBack[16];
    memset(readBack, 0x99, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(layout.read_record(1, readBack, sizeof(readBack), len, version), PDI_ERR_CRC);

    for (uint8_t i = 0; i < sizeof(readBack); i++)
    {
        ASSERT_EQ(readBack[i], (uint8_t)0x99);
    }
}

TEST(dblayout, a_torn_superblock_is_laid_down_fresh_instead_of_being_trusted)
{
    FakeDbStore store;
    pdiutil::vector<struct_tables> tables = twoTables();

    {
        DbLayout layout;
        layout.mount(&store, tables);
    }

    store.m_bytes[DB_REGION_START + 6] ^= 0xFF;

    DbLayout remounted;
    ASSERT_EQ(remounted.mount(&store, tables), PDI_OK);
    ASSERT_TRUE(remounted.was_formatted());
}

TEST(dblayout, clearing_a_record_returns_it_to_empty)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = twoTables();
    layout.mount(&store, tables);

    uint8_t written[16];
    memset(written, 0x33, sizeof(written));
    layout.write_record(1, written, sizeof(written), 1);

    ASSERT_EQ(layout.clear_record(1), PDI_OK);

    uint8_t readBack[16];
    memset(readBack, 0x55, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(layout.read_record(1, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_EQ(len, (uint16_t)0);
    ASSERT_EQ(readBack[0], (uint8_t)0x55);
}

TEST(dblayout, the_superblock_carries_the_firmware_version_across_a_remount)
{
    FakeDbStore store;
    pdiutil::vector<struct_tables> tables = twoTables();

    {
        DbLayout layout;
        layout.mount(&store, tables);
        layout.set_firmware_version(2019041100);
        layout.set_launch_year(26);
    }

    DbLayout remounted;
    remounted.mount(&store, tables);

    ASSERT_EQ(remounted.firmware_version(), (uint32_t)2019041100);
    ASSERT_EQ(remounted.launch_year(), (uint8_t)26);
}

TEST(dblayout, a_table_set_that_cannot_fit_is_refused)
{
    FakeDbStore store;
    DbLayout layout;

    pdiutil::vector<struct_tables> tables;
    tables.push_back(makeTable(1, 4000));
    tables.push_back(makeTable(2, 4000));

    ASSERT_EQ(layout.mount(&store, tables), PDI_ERR_NO_SPACE);
}

TEST(dblayout, reading_before_a_mount_is_refused)
{
    DbLayout layout;

    uint8_t buf[16];
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(layout.read_record(1, buf, sizeof(buf), len, version), PDI_ERR_NOT_INITIALIZED);
}

TEST(dblayout, used_and_free_bytes_account_for_every_record)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = twoTables(16, 32);
    layout.mount(&store, tables);

    ASSERT_EQ(layout.used_bytes(), (uint16_t)48);
    ASSERT_EQ(layout.free_bytes(), (uint16_t)(FakeDbStore::SIZE - DB_PAYLOAD_START - 48));
}

/**
 * The provisioning path: what the device runs is snapshotted to the eeprom, and
 * a container that gets wiped comes back holding those values rather than blank
 * ones.
 */
TEST(dbtier, a_wiped_container_comes_back_from_the_saved_defaults)
{
    FakeDbStore live;
    FakeDbStore defaults;
    pdiutil::vector<struct_tables> tables = twoTables();

    uint8_t first[16];
    uint8_t second[32];
    memset(first, 0xAA, sizeof(first));
    memset(second, 0xBB, sizeof(second));

    DbLayout running;
    running.mount(&live, tables);
    running.write_record(1, first, sizeof(first), 1);
    running.write_record(2, second, sizeof(second), 1);

    {
        DbLayout saved;
        ASSERT_EQ(saved.mount(&defaults, tables), PDI_OK);
        ASSERT_EQ(saved.copy_from(running), PDI_OK);
    }

    memset(live.m_bytes, 0xFF, FakeDbStore::SIZE);

    DbLayout rebuilt;
    ASSERT_EQ(rebuilt.mount(&live, tables), PDI_OK);
    ASSERT_TRUE(rebuilt.was_formatted());

    {
        DbLayout saved;
        ASSERT_EQ(saved.mount(&defaults, tables), PDI_OK);
        ASSERT_FALSE(saved.was_formatted());
        ASSERT_EQ(rebuilt.copy_from(saved), PDI_OK);
    }

    uint8_t readBack[32];
    uint16_t len = 0;
    uint16_t version = 0;

    memset(readBack, 0, sizeof(readBack));
    ASSERT_EQ(rebuilt.read_record(1, readBack, 16, len, version), PDI_OK);
    ASSERT_EQ(len, (uint16_t)16);
    ASSERT_MEMEQ(readBack, first, sizeof(first));

    memset(readBack, 0, sizeof(readBack));
    ASSERT_EQ(rebuilt.read_record(2, readBack, 32, len, version), PDI_OK);
    ASSERT_MEMEQ(readBack, second, sizeof(second));
}

/**
 * A factory reset puts back what was provisioned, which is the point of keeping
 * a defaults tier rather than simply blanking every record.
 */
TEST(dbtier, restoring_defaults_undoes_a_later_change)
{
    FakeDbStore live;
    FakeDbStore defaults;
    pdiutil::vector<struct_tables> tables = twoTables();

    DbLayout running;
    running.mount(&live, tables);

    uint8_t provisioned[16];
    memset(provisioned, 0x11, sizeof(provisioned));
    running.write_record(1, provisioned, sizeof(provisioned), 1);

    {
        DbLayout saved;
        saved.mount(&defaults, tables);
        saved.copy_from(running);
    }

    uint8_t changed[16];
    memset(changed, 0x99, sizeof(changed));
    running.write_record(1, changed, sizeof(changed), 1);

    {
        DbLayout saved;
        saved.mount(&defaults, tables);
        ASSERT_EQ(running.copy_from(saved), PDI_OK);
    }

    uint8_t readBack[16];
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(running.read_record(1, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_MEMEQ(readBack, provisioned, sizeof(provisioned));
}

/**
 * A device that was never provisioned has empty defaults, and restoring them
 * has to return the live record to empty so the struct defaults take over.
 */
TEST(dbtier, empty_defaults_clear_the_live_record)
{
    FakeDbStore live;
    FakeDbStore defaults;
    pdiutil::vector<struct_tables> tables = twoTables();

    DbLayout running;
    running.mount(&live, tables);

    uint8_t written[16];
    memset(written, 0x77, sizeof(written));
    running.write_record(1, written, sizeof(written), 1);

    {
        DbLayout saved;
        saved.mount(&defaults, tables);
        ASSERT_EQ(running.copy_from(saved), PDI_OK);
    }

    uint8_t readBack[16];
    memset(readBack, 0x5C, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(running.read_record(1, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_EQ(len, (uint16_t)0);
    ASSERT_EQ(readBack[0], (uint8_t)0x5C);
}

/**
 * Firmware that grew a struct still takes the shorter payload the defaults tier
 * is holding, and leaves the added tail at its default.
 */
TEST(dbtier, defaults_written_by_an_older_struct_still_restore)
{
    FakeDbStore live;
    FakeDbStore defaults;
    pdiutil::vector<struct_tables> before = twoTables(16, 32);

    {
        DbLayout saved;
        saved.mount(&defaults, before);
        uint8_t written[16];
        memset(written, 0x31, sizeof(written));
        saved.write_record(1, written, sizeof(written), 1);
    }

    pdiutil::vector<struct_tables> grown = twoTables(24, 32);
    DbLayout running;
    ASSERT_EQ(running.mount(&live, grown), PDI_OK);

    {
        DbLayout saved;
        ASSERT_EQ(saved.mount(&defaults, grown), PDI_OK);
        ASSERT_EQ(running.copy_from(saved), PDI_OK);
    }

    uint8_t readBack[24];
    memset(readBack, 0xE1, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(running.read_record(1, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_EQ(len, (uint16_t)16);
    ASSERT_EQ(readBack[0], (uint8_t)0x31);
    ASSERT_EQ(readBack[16], (uint8_t)0xE1);
}

/**
 * The point of sealing: a copy of the database taken off the device must not
 * carry the credential in the clear.
 */
TEST(dbseal, a_sealed_record_leaves_no_plaintext_in_the_store)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = sealedTables();
    ASSERT_EQ(layout.mount(&store, tables), PDI_OK);

    const db_dir_entry_t *entry = layout.entry_at(0);
    ASSERT_NOT_NULL(entry);
    ASSERT_TRUE(0 != (entry->m_flags & DB_ENTRY_FLAG_SEALED));
    ASSERT_EQ(entry->m_cap, (uint16_t)(64 + DB_SEAL_OVERHEAD));

    uint8_t secret[64];
    for (uint8_t i = 0; i < sizeof(secret); i++) secret[i] = (uint8_t)('A' + (i % 26));

    ASSERT_EQ(layout.write_record(1, secret, sizeof(secret), 1), PDI_OK);

    bool leaked = false;
    for (uint32_t i = 0; i + 8 <= FakeDbStore::SIZE; i++)
    {
        if (0 == memcmp(&store.m_bytes[i], secret, 8)) { leaked = true; break; }
    }
    ASSERT_FALSE(leaked);

    uint8_t readBack[64];
    memset(readBack, 0, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(layout.read_record(1, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_EQ(len, (uint16_t)64);
    ASSERT_MEMEQ(readBack, secret, sizeof(secret));
}

TEST(dbseal, tampered_ciphertext_is_refused_without_touching_the_caller)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = sealedTables();
    layout.mount(&store, tables);

    uint8_t secret[64];
    memset(secret, 0x5A, sizeof(secret));
    layout.write_record(1, secret, sizeof(secret), 1);

    const db_dir_entry_t *entry = layout.entry_at(0);
    store.m_bytes[entry->m_offset + DB_SEAL_IV_BYTES + 5] ^= 0x01;

    uint8_t readBack[64];
    memset(readBack, 0xD7, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(layout.read_record(1, readBack, sizeof(readBack), len, version), PDI_ERR_CRC);

    for (uint8_t i = 0; i < sizeof(readBack); i++)
    {
        ASSERT_EQ(readBack[i], (uint8_t)0xD7);
    }
}

TEST(dbseal, a_tampered_tag_is_refused)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = sealedTables();
    layout.mount(&store, tables);

    uint8_t secret[64];
    memset(secret, 0x11, sizeof(secret));
    layout.write_record(1, secret, sizeof(secret), 1);

    const db_dir_entry_t *entry = layout.entry_at(0);
    store.m_bytes[entry->m_offset + DB_SEAL_IV_BYTES + 64 + 3] ^= 0xFF;

    uint8_t readBack[64];
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(layout.read_record(1, readBack, sizeof(readBack), len, version), PDI_ERR_CRC);
}

/**
 * Every write takes a fresh nonce, so storing the same credential twice does
 * not produce the same bytes and cannot be recognised on the medium.
 */
TEST(dbseal, the_same_value_sealed_twice_differs_on_the_medium)
{
    FakeDbStore firstStore;
    FakeDbStore secondStore;
    pdiutil::vector<struct_tables> tablesOne = sealedTables();
    pdiutil::vector<struct_tables> tablesTwo = sealedTables();

    DbLayout first;
    DbLayout second;
    first.mount(&firstStore, tablesOne);
    second.mount(&secondStore, tablesTwo);

    uint8_t secret[64];
    memset(secret, 0x77, sizeof(secret));
    first.write_record(1, secret, sizeof(secret), 1);
    second.write_record(1, secret, sizeof(secret), 1);

    const db_dir_entry_t *entry = first.entry_at(0);
    ASSERT_NE(memcmp(&firstStore.m_bytes[entry->m_offset], &secondStore.m_bytes[entry->m_offset],
                     64 + DB_SEAL_OVERHEAD), 0);
}

TEST(dbseal, a_plain_record_beside_a_sealed_one_stays_plain)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = sealedTables();
    layout.mount(&store, tables);

    uint8_t plain[32];
    memset(plain, 0x33, sizeof(plain));
    ASSERT_EQ(layout.write_record(2, plain, sizeof(plain), 1), PDI_OK);

    const db_dir_entry_t *entry = layout.entry_at(1);
    ASSERT_EQ(entry->m_cap, (uint16_t)32);
    ASSERT_EQ(entry->m_flags & DB_ENTRY_FLAG_SEALED, 0);
    ASSERT_MEMEQ(&store.m_bytes[entry->m_offset], plain, sizeof(plain));
}

TEST(dbseal, a_sealed_record_survives_a_repack)
{
    FakeDbStore store;
    pdiutil::vector<struct_tables> before = sealedTables();

    uint8_t secret[64];
    for (uint8_t i = 0; i < sizeof(secret); i++) secret[i] = (uint8_t)(i * 5);

    {
        DbLayout layout;
        layout.mount(&store, before);
        layout.write_record(1, secret, sizeof(secret), 1);
    }

    pdiutil::vector<struct_tables> after;
    after.push_back(makeTable(1, 64, 1, true));
    after.push_back(makeTable(2, 48, 1, false));

    DbLayout grown;
    ASSERT_EQ(grown.mount(&store, after), PDI_OK);

    uint8_t readBack[64];
    memset(readBack, 0, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(grown.read_record(1, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_MEMEQ(readBack, secret, sizeof(secret));
}

TEST(dbseal, sealed_defaults_restore_across_tiers)
{
    FakeDbStore live;
    FakeDbStore defaults;
    pdiutil::vector<struct_tables> tables = sealedTables();

    DbLayout running;
    running.mount(&live, tables);

    uint8_t secret[64];
    memset(secret, 0x2B, sizeof(secret));
    running.write_record(1, secret, sizeof(secret), 1);

    {
        DbLayout saved;
        saved.mount(&defaults, tables);
        ASSERT_EQ(saved.copy_from(running), PDI_OK);
    }

    memset(live.m_bytes, 0xFF, FakeDbStore::SIZE);

    DbLayout rebuilt;
    ASSERT_EQ(rebuilt.mount(&live, tables), PDI_OK);

    {
        DbLayout saved;
        saved.mount(&defaults, tables);
        ASSERT_EQ(rebuilt.copy_from(saved), PDI_OK);
    }

    uint8_t readBack[64];
    memset(readBack, 0, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(rebuilt.read_record(1, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_MEMEQ(readBack, secret, sizeof(secret));
}

/**
 * A store that hands a range over in one call and in odd-sized pieces, the way
 * a filesystem returns its own chunks, and counts how the engine reads it.
 */
struct StreamingDbStore : public FakeDbStore
{
    uint32_t m_reads;
    uint32_t m_streams;

    StreamingDbStore() : m_reads(0), m_streams(0)
    {
    }

    bool read(uint32_t offset, uint8_t *buf, uint32_t len) override
    {
        m_reads++;
        return FakeDbStore::read(offset, buf, len);
    }

    bool read_each(uint32_t offset, uint32_t len, uint8_t *, uint16_t,
                   pdiutil::function<void(const uint8_t *, uint32_t)> fn) override
    {
        m_streams++;
        if (offset + len > SIZE) return false;

        for (uint32_t at = 0; at < len; at += 7)
        {
            fn(&m_bytes[offset + at], (len - at) > 7 ? 7 : (len - at));
        }
        return true;
    }
};

TEST(dblayout, a_plain_record_is_checked_through_one_stream)
{
    StreamingDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = twoTables(100, 32);
    layout.mount(&store, tables);

    uint8_t written[100];
    for (uint8_t i = 0; i < sizeof(written); i++) written[i] = i;
    layout.write_record(1, written, sizeof(written), 1);

    store.m_reads = 0;
    store.m_streams = 0;

    uint8_t readBack[100];
    memset(readBack, 0, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(layout.read_record(1, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_MEMEQ(readBack, written, sizeof(written));
    ASSERT_EQ(store.m_streams, (uint32_t)1);
    ASSERT_EQ(store.m_reads, (uint32_t)1);

    const db_dir_entry_t *entry = layout.entry_at(0);

    store.m_bytes[entry->m_offset + 99] ^= 0x01;
    ASSERT_EQ(layout.verify_record(1), PDI_ERR_CRC);
    store.m_bytes[entry->m_offset + 99] ^= 0x01;
    ASSERT_EQ(layout.verify_record(1), PDI_OK);

    store.m_bytes[entry->m_offset + 40] ^= 0x01;
    ASSERT_EQ(layout.verify_record(1), PDI_ERR_CRC);
}

TEST(dblayout, a_byte_past_the_first_window_is_checked_by_the_default_read)
{
    FakeDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = twoTables(100, 32);
    layout.mount(&store, tables);

    uint8_t written[100];
    for (uint8_t i = 0; i < sizeof(written); i++) written[i] = i;
    layout.write_record(1, written, sizeof(written), 1);
    ASSERT_EQ(layout.verify_record(1), PDI_OK);

    const db_dir_entry_t *entry = layout.entry_at(0);

    store.m_bytes[entry->m_offset + 70] ^= 0x01;
    ASSERT_EQ(layout.verify_record(1), PDI_ERR_CRC);
    store.m_bytes[entry->m_offset + 70] ^= 0x01;

    store.m_bytes[entry->m_offset + 99] ^= 0x01;
    ASSERT_EQ(layout.verify_record(1), PDI_ERR_CRC);
}

TEST(dbseal, a_sealed_record_is_checked_through_one_stream)
{
    StreamingDbStore store;
    DbLayout layout;
    pdiutil::vector<struct_tables> tables = sealedTables(100);
    layout.mount(&store, tables);

    uint8_t secret[100];
    for (uint8_t i = 0; i < sizeof(secret); i++) secret[i] = (uint8_t)(0xA0 ^ i);
    layout.write_record(1, secret, sizeof(secret), 1);

    store.m_reads = 0;
    store.m_streams = 0;

    uint8_t readBack[100];
    memset(readBack, 0, sizeof(readBack));
    uint16_t len = 0;
    uint16_t version = 0;

    ASSERT_EQ(layout.read_record(1, readBack, sizeof(readBack), len, version), PDI_OK);
    ASSERT_MEMEQ(readBack, secret, sizeof(secret));
    ASSERT_EQ(store.m_streams, (uint32_t)1);
    ASSERT_EQ(store.m_reads, (uint32_t)3);

    const db_dir_entry_t *entry = layout.entry_at(0);

    store.m_bytes[entry->m_offset + 3] ^= 0x01;
    ASSERT_EQ(layout.read_record(1, readBack, sizeof(readBack), len, version), PDI_ERR_CRC);
    store.m_bytes[entry->m_offset + 3] ^= 0x01;

    store.m_bytes[entry->m_offset + DB_SEAL_IV_BYTES + 90] ^= 0x01;
    ASSERT_EQ(layout.read_record(1, readBack, sizeof(readBack), len, version), PDI_ERR_CRC);
}

TEST(checksum, crc16_changes_with_a_single_flipped_bit)
{
    uint8_t data[8];
    memset(data, 0x10, sizeof(data));

    uint16_t before = crc16Ccitt(data, sizeof(data));
    data[4] ^= 0x01;

    ASSERT_NE(crc16Ccitt(data, sizeof(data)), before);
}

TEST(checksum, crc16_of_zeroes_is_not_zero)
{
    uint8_t data[8];
    memset(data, 0, sizeof(data));

    ASSERT_NE(crc16Ccitt(data, sizeof(data)), (uint16_t)0);
}

/**
 * The engine checksums a record through a small window, so a split run has to
 * land on the same value as one pass over the whole range.
 */
TEST(checksum, crc16_can_be_continued_across_buffers)
{
    uint8_t data[64];
    for (uint8_t i = 0; i < sizeof(data); i++) data[i] = (uint8_t)(i * 7);

    uint16_t whole = crc16Ccitt(data, sizeof(data));

    uint16_t split = crc16CcittUpdate(0xFFFF, data, 32);
    split = crc16CcittUpdate(split, &data[32], 32);

    ASSERT_EQ(split, whole);
}

/**
 * The one shot form is the streaming one underneath, so a mac taken in pieces
 * has to match a mac taken in a single pass.
 */
TEST(hmac, streaming_matches_a_single_pass)
{
    uint8_t key[20];
    memset(key, 0x0b, sizeof(key));

    uint8_t data[200];
    for (uint8_t i = 0; i < sizeof(data); i++) data[i] = (uint8_t)(i * 3);

    uint8_t whole[32];
    uint8_t split[32];

    hmac_sha256(key, sizeof(key), data, sizeof(data), whole);

    hmac_sha256_context ctx;
    hmac_sha256_init(&ctx, key, sizeof(key));
    hmac_sha256_update(&ctx, data, 64);
    hmac_sha256_update(&ctx, &data[64], 64);
    hmac_sha256_update(&ctx, &data[128], 72);
    hmac_sha256_final(&ctx, split);

    ASSERT_MEMEQ(split, whole, 32);
}
