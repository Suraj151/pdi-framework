/************************* Database Command Tests *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The db command against the record store the stack really runs on. Each verb is
checked against what the store itself reports, so a listing that drifts from
the status line, or a verify that passes a record that has rotted, shows up as
a disagreement rather than as a line that merely looks right.

Author          : Suraj I.
created Date    : 8th Oct 2026
******************************************************************************/

#include <ShellHarness.h>
#include <pditest.h>

#if defined(ENABLE_STORAGE_SERVICE)

#include <helpers/FeatureConfigFiles.h>
#include <string>
#include <vector>

using pditest::saw;

namespace
{

    /**
     * The number that follows a "name : " label in db status, or -1.
     */
    long statusValue(const std::string &out, const char *label)
    {
        size_t at = out.find(label);
        if (std::string::npos == at)
        {
            return -1;
        }
        at = out.find(':', at);
        if (std::string::npos == at)
        {
            return -1;
        }
        return strtol(out.c_str() + at + 1, nullptr, 10);
    }

    /**
     * The listing's record rows, the header left out.
     */
    std::vector<std::string> listRows(const std::string &out)
    {
        std::vector<std::string> rows;
        size_t start = 0;
        while (start < out.size())
        {
            size_t end = out.find('\n', start);
            if (std::string::npos == end)
            {
                end = out.size();
            }
            std::string line = out.substr(start, end - start);
            if (!line.empty() && '\r' == line.back())
            {
                line.pop_back();
            }
            if (!line.empty() && isdigit((unsigned char)line[0]))
            {
                rows.push_back(line);
            }
            start = end + 1;
        }
        return rows;
    }

    /**
     * Flips one byte of the store file in place. Calling it twice on the same
     * offset puts the byte back.
     */
    bool flipStoreByte(uint32_t offset)
    {
        pdi_fhandle_t handle = __i_fs.openFile(DB_STORE_FILE, FILE_OPEN_READ | FILE_OPEN_WRITE);
        if (handle < 0)
        {
            return false;
        }

        char byte = 0;
        bool done = __i_fs.seekFile(handle, offset, FILE_SEEK_SET) >= 0 &&
                    1 == __i_fs.readFileHandle(handle, &byte, 1);
        byte = (char)(byte ^ 0xFF);
        done = done && __i_fs.seekFile(handle, offset, FILE_SEEK_SET) >= 0 &&
               1 == __i_fs.writeFileHandle(handle, &byte, 1);

        __i_fs.closeFile(handle);
        return done;
    }

    /**
     * The first record that holds bytes of its own, or null.
     */
    const db_dir_entry_t *aWrittenRecord()
    {
        for (uint8_t i = 0; i < __db_layout.entry_count(); i++)
        {
            const db_dir_entry_t *entry = __db_layout.entry_at(i);
            if (nullptr != entry && entry->m_len > 2)
            {
                return entry;
            }
        }
        return nullptr;
    }
}

TEST(cmddb, db_without_a_verb_asks_for_one)
{
    pditest::mountedDb();
    pditest::Shell sh;

    std::string out = sh.run("db");

    ASSERT_EQ((int)sh.result(), (int)CMD_ERROR_ARGS_MISSING);
    ASSERT_TRUE(saw(out, "status|list|verify|save|restore"));
}

TEST(cmddb, db_refuses_a_verb_it_does_not_know)
{
    pditest::mountedDb();
    pditest::Shell sh;

    sh.run("db bogus");
    ASSERT_EQ((int)sh.result(), (int)CMD_ERROR_OPT);

    sh.run("db stat");
    ASSERT_EQ((int)sh.result(), (int)CMD_ERROR_OPT);

    sh.run("db statusx");
    ASSERT_EQ((int)sh.result(), (int)CMD_ERROR_OPT);
}

TEST(cmddb, db_status_reports_the_live_medium_and_its_fill)
{
    pditest::mountedDb();
    pditest::Shell sh;

    std::string out = sh.run("db status");

    ASSERT_EQ((int)sh.result(), (int)PDI_OK);
    ASSERT_TRUE(saw(out, "medium"));
    ASSERT_TRUE(saw(out, __database_service.is_storage_tier() ? DB_STORE_FILE : "eeprom"));
    ASSERT_TRUE(saw(out, "mounted  : yes"));
    ASSERT_EQ(statusValue(out, "records"), (long)__db_layout.entry_count());
    ASSERT_GT(statusValue(out, "used"), 0L);
    ASSERT_GE(statusValue(out, "free"), 0L);
    ASSERT_EQ(statusValue(out, "firmware"), (long)FIRMWARE_VERSION);
}

TEST(cmddb, db_list_has_one_row_per_record_the_status_counts)
{
    pditest::mountedDb();
    pditest::Shell sh;

    long records = statusValue(sh.run("db status"), "records");
    std::string out = sh.run("db list");

    ASSERT_EQ((int)sh.result(), (int)PDI_OK);
    ASSERT_TRUE(saw(out, "ID"));
    ASSERT_TRUE(saw(out, "STATE"));

    std::vector<std::string> rows = listRows(out);
    ASSERT_EQ((long)rows.size(), records);

    for (size_t i = 0; i < rows.size(); i++)
    {
        const std::string &row = rows[i];
        bool known = saw(row, "default") || saw(row, "sealed") || saw(row, "stored");
        ASSERT_TRUE(known);
        ASSERT_EQ(strtol(row.c_str(), nullptr, 10), (long)__db_layout.entry_at((uint8_t)i)->m_id);
    }
}

TEST(cmddb, db_list_never_prints_a_records_contents)
{
    pditest::mountedDb();
    pditest::Shell sh;

    // the wifi password is one of the sealed records, and a listing is for
    // seeing what is stored, not what it says
    wifi_config_table wifi;
    __database_service.get_wifi_config_table(&wifi);

    std::string out = sh.run("db list");

    if (strlen(wifi.sta_password) > 0)
    {
        ASSERT_FALSE(saw(out, wifi.sta_password));
    }
    if (strlen(wifi.ap_password) > 0)
    {
        ASSERT_FALSE(saw(out, wifi.ap_password));
    }
}

TEST(cmddb, db_verify_passes_a_store_nothing_has_touched)
{
    pditest::mountedDb();
    pditest::Shell sh;

    std::string out = sh.run("db verify");

    ASSERT_EQ((int)sh.result(), (int)PDI_OK);
    ASSERT_TRUE(saw(out, "every record verified"));
}

TEST(cmddb, db_verify_names_a_record_that_has_rotted_and_fails)
{
    pditest::mountedDb();
    pditest::Shell sh;

    // a fresh store holds nothing but defaults, so one record is written first
    // to give the rot something to land in
    ota_config_table ota;
    __database_service.get_ota_config_table(&ota);
    ASSERT_TRUE(__database_service.set_ota_config_table(&ota));

    const db_dir_entry_t *entry = aWrittenRecord();
    ASSERT_NOT_NULL(entry);
    uint16_t id = entry->m_id;
    uint32_t offset = (uint32_t)entry->m_offset + 1;

    ASSERT_TRUE(flipStoreByte(offset));
    std::string rotted = sh.run("db verify");
    pdi_err_t rottedresult = sh.result();
    ASSERT_TRUE(flipStoreByte(offset));

    ASSERT_EQ((int)rottedresult, (int)CMD_ERROR_FAILED);
    ASSERT_TRUE(saw(rotted, ("record " + std::to_string(id) + " failed").c_str()));
    ASSERT_FALSE(saw(rotted, "every record verified"));

    std::string healed = sh.run("db verify");
    ASSERT_EQ((int)sh.result(), (int)PDI_OK);
    ASSERT_TRUE(saw(healed, "every record verified"));
}

#ifdef ENABLE_GPIO_SERVICE
TEST(cmddb, db_restore_brings_back_what_db_save_took)
{
    pditest::mountedDb();
    pditest::Shell sh;

    // a record with no conf file over it, so what is read back is the record
    gpio_config_table gpio;
    ASSERT_TRUE(__database_service.get_gpio_config_table(&gpio));
    uint32_t original = gpio.gpio_post_frequency;

    gpio.gpio_post_frequency = 41000;
    ASSERT_TRUE(__database_service.set_gpio_config_table(&gpio));

    std::string saved = sh.run("db save");
    ASSERT_EQ((int)sh.result(), (int)PDI_OK);
    ASSERT_TRUE(saw(saved, "defaults saved"));

    gpio.gpio_post_frequency = 42000;
    ASSERT_TRUE(__database_service.set_gpio_config_table(&gpio));

    std::string restored = sh.run("db restore");
    ASSERT_EQ((int)sh.result(), (int)PDI_OK);
    ASSERT_TRUE(saw(restored, "defaults restored"));

    gpio_config_table after;
    ASSERT_TRUE(__database_service.get_gpio_config_table(&after));
    uint32_t seen = after.gpio_post_frequency;

    after.gpio_post_frequency = original;
    __database_service.set_gpio_config_table(&after);
    sh.run("db save");

    ASSERT_EQ(seen, (uint32_t)41000);
}
#endif

TEST(cmddb, db_needs_an_authorised_session)
{
    pditest::mountedDb();
    pditest::Shell sh;

    __auth_service.setAuthorized(false);
    std::string out = sh.run("db status");
    pdi_err_t result = sh.result();
    __auth_service.setAuthorized(true);

    ASSERT_FALSE(saw(out, "medium"));
    ASSERT_NE((int)result, (int)PDI_OK);
}

#if defined(ENABLE_OTA_SERVICE) && defined(ENABLE_OTA_CONFIG_FILE)
TEST(cmddb, a_restored_record_is_not_undone_by_its_conf_file_at_the_next_start)
{
    pditest::mountedDb();
    pditest::Shell sh;

    ota_config_table original;
    ASSERT_TRUE(__database_service.get_ota_config_table(&original));

    ota_config_table ota;
    strcpy(ota.ota_host, "http://saved.example:81");
    ASSERT_TRUE(__database_service.set_ota_config_table(&ota));
    sh.run("db save");

    memset(ota.ota_host, 0, sizeof(ota.ota_host));
    strcpy(ota.ota_host, "http://newer.example:82");
    ASSERT_TRUE(__database_service.set_ota_config_table(&ota));

    sh.run("db restore");
    syncOtaConfigFile();

    ota_config_table after;
    __database_service.get_ota_config_table(&after);
    std::string seen = after.ota_host;

    __database_service.set_ota_config_table(&original);
    sh.run("db save");

    ASSERT_STREQ(seen.c_str(), "http://saved.example:81");
}
#endif

#endif
