/****************************** Database Tests ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <utility/Database.h>

/**
 * A table that records whether the engine booted and cleared it.
 *
 * DatabaseTableAbstractLayer registers itself into a static list on
 * construction and never leaves it, so instances have to outlive every test the
 * way the framework's own tables are globals.
 */
struct CountingTable : public DatabaseTableAbstractLayer
{
    int boots;
    int clears;

    CountingTable() : boots(0), clears(0) {}

    void boot() override { boots++; }
    void clear() override { clears++; }

    void forget()
    {
        boots = 0;
        clears = 0;
    }
};

static CountingTable s_table_one;
static CountingTable s_table_two;

static struct_tables makeTable(uint16_t address, uint16_t size,
                               DatabaseTableAbstractLayer *instance = nullptr)
{
    struct_tables table;
    memset(&table, 0, sizeof(table));
    table.m_table_address = address;
    table.m_table_size = size;
    table.m_instance = instance;
    return table;
}

TEST(database, starts_with_no_tables)
{
    Database db;
    ASSERT_EQ(db.m_database_tables.size(), (size_t)0);
}

TEST(database, last_table_of_an_empty_database_is_zeroed)
{
    Database db;
    struct_tables last = db.get_last_table();

    ASSERT_EQ(last.m_table_address, (uint16_t)0);
    ASSERT_EQ(last.m_table_size, (uint16_t)0);
}

TEST(database, registers_a_table_that_fits)
{
    Database db;
    db.init_database(4096);

    struct_tables table = makeTable(100, 50);
    ASSERT_TRUE(db.register_table(table));
    ASSERT_EQ(db.m_database_tables.size(), (size_t)1);
}

TEST(database, registers_successive_non_overlapping_tables)
{
    Database db;
    db.init_database(4096);

    struct_tables first = makeTable(100, 50);
    struct_tables second = makeTable(200, 50);
    struct_tables third = makeTable(300, 50);

    ASSERT_TRUE(db.register_table(first));
    ASSERT_TRUE(db.register_table(second));
    ASSERT_TRUE(db.register_table(third));
    ASSERT_EQ(db.m_database_tables.size(), (size_t)3);
}

TEST(database, refuses_a_table_that_overlaps_the_previous_one)
{
    Database db;
    db.init_database(4096);

    struct_tables first = makeTable(100, 100);
    struct_tables overlapping = makeTable(150, 50);

    ASSERT_TRUE(db.register_table(first));
    ASSERT_FALSE(db.register_table(overlapping));
    ASSERT_EQ(db.m_database_tables.size(), (size_t)1);
}

TEST(database, refuses_a_table_starting_at_the_previous_end)
{
    Database db;
    db.init_database(4096);

    struct_tables first = makeTable(100, 50);
    struct_tables abutting = makeTable(150, 50);

    ASSERT_TRUE(db.register_table(first));
    ASSERT_FALSE(db.register_table(abutting));
}

TEST(database, refuses_a_table_that_runs_past_the_database_size)
{
    Database db;
    db.init_database(512);

    struct_tables toobig = makeTable(400, 200);
    ASSERT_FALSE(db.register_table(toobig));
}

TEST(database, refuses_every_table_when_the_size_is_zero)
{
    Database db;
    db.init_database(0);

    struct_tables table = makeTable(10, 10);
    ASSERT_FALSE(db.register_table(table));
}

TEST(database, last_table_reports_the_highest_address)
{
    Database db;
    db.init_database(4096);

    struct_tables first = makeTable(100, 50);
    struct_tables second = makeTable(200, 60);
    struct_tables third = makeTable(400, 70);

    db.register_table(first);
    db.register_table(second);
    db.register_table(third);

    struct_tables last = db.get_last_table();
    ASSERT_EQ(last.m_table_address, (uint16_t)400);
    ASSERT_EQ(last.m_table_size, (uint16_t)70);
}

TEST(database, clear_all_reaches_every_registered_instance)
{
    Database db;
    db.init_database(4096);
    s_table_one.forget();
    s_table_two.forget();

    struct_tables first = makeTable(100, 50, &s_table_one);
    struct_tables second = makeTable(200, 50, &s_table_two);

    db.register_table(first);
    db.register_table(second);
    db.clear_all();

    ASSERT_EQ(s_table_one.clears, 1);
    ASSERT_EQ(s_table_two.clears, 1);
}

TEST(database, clear_all_skips_a_table_with_no_instance)
{
    Database db;
    db.init_database(4096);
    s_table_one.forget();

    struct_tables headless = makeTable(100, 50, nullptr);
    struct_tables attached = makeTable(200, 50, &s_table_one);

    db.register_table(headless);
    db.register_table(attached);
    db.clear_all();

    ASSERT_EQ(s_table_one.clears, 1);
}

TEST(database, init_boots_every_registered_table_instance)
{
    Database db;
    s_table_one.forget();
    s_table_two.forget();

    db.init_database(4096);

    ASSERT_EQ(s_table_one.boots, 1);
    ASSERT_EQ(s_table_two.boots, 1);
}

TEST(database, clear_all_on_an_empty_database_is_harmless)
{
    Database db;
    db.init_database(4096);
    db.clear_all();
    ASSERT_EQ(db.m_database_tables.size(), (size_t)0);
}

/**
 * Tables boot in instance creation order, which the schema does not keep in
 * address order. Every one of them still has to reach the registry, because
 * clear_all only walks registered tables and a factory reset runs through it.
 */
TEST(database, registers_every_table_when_addresses_arrive_out_of_order)
{
    Database db;
    db.init_database(4096);

    const uint16_t addresses[] = {2000, 5, 2600, 500, 50, 700, 1500, 1700, 300, 150};
    const uint16_t size = 40;

    for (uint8_t i = 0; i < (sizeof(addresses) / sizeof(addresses[0])); i++)
    {
        struct_tables table = makeTable(addresses[i], size);
        ASSERT_TRUE(db.register_table(table));
    }

    ASSERT_EQ(db.m_database_tables.size(), (size_t)10);
}

TEST(database, refuses_a_table_overlapping_one_registered_earlier_out_of_order)
{
    Database db;
    db.init_database(4096);

    struct_tables high = makeTable(2000, 40);
    struct_tables low = makeTable(100, 40);
    struct_tables clashesWithLow = makeTable(120, 40);

    ASSERT_TRUE(db.register_table(high));
    ASSERT_TRUE(db.register_table(low));
    ASSERT_FALSE(db.register_table(clashesWithLow));
}

TEST(database, refuses_a_table_that_straddles_an_existing_one)
{
    Database db;
    db.init_database(4096);

    struct_tables inner = makeTable(500, 40);
    struct_tables straddling = makeTable(480, 200);

    ASSERT_TRUE(db.register_table(inner));
    ASSERT_FALSE(db.register_table(straddling));
}

/**
 * How many tables can exist is capped where the instances are counted, not
 * here, so a full set of well spaced tables all reach the registry.
 */
TEST(database, accepts_a_full_set_of_well_spaced_tables)
{
    Database db;
    db.init_database(65000);

    for (uint16_t i = 0; i < MAX_TABLES; i++)
    {
        struct_tables table = makeTable((uint16_t)(100 + (i * 100)), 50);
        ASSERT_TRUE(db.register_table(table));
    }

    ASSERT_EQ(db.m_database_tables.size(), (size_t)MAX_TABLES);
}

TEST(database, the_instance_registry_is_capped_at_max_tables)
{
    ASSERT_LE(DatabaseTableAbstractLayer::m_total_instances, (int)MAX_TABLES);
}
