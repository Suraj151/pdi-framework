/************************** Device IoT Key Tests ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers the key a device proves itself with to its iot server: generated once
at registration, kept while host and duid stay, dropped whichever way either
of them changes, since every way of changing them stores through the record.

Author          : Suraj I.
created Date    : 3rd Oct 2026
******************************************************************************/

#include <MountedStack.h>
#include <pditest.h>

#include <service_provider/database/DatabaseServiceProvider.h>

#if defined(ENABLE_DEVICE_IOT)

#include <service_provider/iot/DeviceIotServiceProvider.h>

namespace
{

const char *HOST = "https://iot.example";
const char *DUID = "00000001-SENSOR01-R01-C01-2026-000001";
const char *KEY = "00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff";

/**
 * A device iot record carrying the given host, duid and key.
 */
device_iot_config_table configs(const char *host, const char *duid, const char *key)
{
    device_iot_config_table table;
    strncpy(table.device_iot_host, host, DEVICE_IOT_HOST_BUF_SIZE - 1);
    strncpy(table.device_iot_duid, duid, DEVICE_IOT_DUID_MAX_LENGTH - 1);
    strncpy(table.device_iot_key, key, DEVICE_IOT_KEY_BUF_SIZE - 1);
    return table;
}

/**
 * Store the default host and duid with the given key, and return that record.
 */
device_iot_config_table seed(const char *key)
{
    pditest::mountedDb();

    device_iot_config_table table = configs(HOST, DUID, "");
    __database_service.set_device_iot_config_table(&table);

    table = configs(HOST, DUID, key);
    __database_service.set_device_iot_config_table(&table);
    return table;
}

/**
 * The device iot record as the store holds it now.
 */
device_iot_config_table stored()
{
    device_iot_config_table table;
    __database_service.get_device_iot_config_table(&table);
    return table;
}

/**
 * True when every character of value is a hex digit.
 */
bool isHex(const char *value)
{
    for (const char *c = value; *c; c++)
    {
        if (!((*c >= '0' && *c <= '9') || (*c >= 'a' && *c <= 'f') || (*c >= 'A' && *c <= 'F')))
        {
            return false;
        }
    }
    return true;
}

} // namespace

TEST(deviceiotkey, the_key_is_kept_while_host_and_duid_stay)
{
    device_iot_config_table table = seed(KEY);
    ASSERT_STREQ(stored().device_iot_key, KEY);

    ASSERT_TRUE(__database_service.set_device_iot_config_table(&table));
    ASSERT_STREQ(table.device_iot_key, KEY);
    ASSERT_STREQ(stored().device_iot_key, KEY);
}

TEST(deviceiotkey, a_new_host_drops_the_key)
{
    seed(KEY);

    device_iot_config_table table = stored();
    strncpy(table.device_iot_host, "https://other.example", DEVICE_IOT_HOST_BUF_SIZE - 1);
    ASSERT_TRUE(__database_service.set_device_iot_config_table(&table));

    ASSERT_STREQ(table.device_iot_key, "");
    ASSERT_STREQ(stored().device_iot_key, "");
    ASSERT_STREQ(stored().device_iot_host, "https://other.example");
}

TEST(deviceiotkey, a_new_duid_drops_the_key)
{
    seed(KEY);

    device_iot_config_table table = stored();
    memset(table.device_iot_duid, 0, DEVICE_IOT_DUID_MAX_LENGTH);
    strncpy(table.device_iot_duid, "00000002-SENSOR01-R01-C01-2026-000002", DEVICE_IOT_DUID_MAX_LENGTH - 1);
    ASSERT_TRUE(__database_service.set_device_iot_config_table(&table));

    ASSERT_STREQ(stored().device_iot_key, "");
}

TEST(deviceiotkey, a_key_written_together_with_a_new_host_is_not_taken)
{
    seed(KEY);

    device_iot_config_table table = configs("https://other.example", DUID, KEY);
    ASSERT_TRUE(__database_service.set_device_iot_config_table(&table));

    ASSERT_STREQ(stored().device_iot_key, "");
}

TEST(deviceiotkey, registration_generates_a_key_once_and_stores_it)
{
    device_iot_config_table table = seed("");

    ASSERT_TRUE(__device_iot_service.ensureDeviceKey(&table));
    ASSERT_EQ(strlen(table.device_iot_key), (size_t)(DEVICE_IOT_KEY_BYTES * 2));
    ASSERT_TRUE(isHex(table.device_iot_key));
    ASSERT_STREQ(stored().device_iot_key, table.device_iot_key);

    char first[DEVICE_IOT_KEY_BUF_SIZE];
    memcpy(first, table.device_iot_key, DEVICE_IOT_KEY_BUF_SIZE);

    ASSERT_TRUE(__device_iot_service.ensureDeviceKey(&table));
    ASSERT_STREQ(table.device_iot_key, first);
    ASSERT_STREQ(stored().device_iot_key, first);
}

TEST(deviceiotkey, a_changed_duid_gets_a_new_key_at_the_next_registration)
{
    device_iot_config_table table = seed("");
    ASSERT_TRUE(__device_iot_service.ensureDeviceKey(&table));

    char first[DEVICE_IOT_KEY_BUF_SIZE];
    memcpy(first, table.device_iot_key, DEVICE_IOT_KEY_BUF_SIZE);

    memset(table.device_iot_duid, 0, DEVICE_IOT_DUID_MAX_LENGTH);
    strncpy(table.device_iot_duid, "00000003-SENSOR01-R01-C01-2026-000003", DEVICE_IOT_DUID_MAX_LENGTH - 1);
    ASSERT_TRUE(__database_service.set_device_iot_config_table(&table));
    ASSERT_STREQ(table.device_iot_key, "");

    ASSERT_TRUE(__device_iot_service.ensureDeviceKey(&table));
    ASSERT_STRNE(table.device_iot_key, first);
    ASSERT_STREQ(stored().device_iot_key, table.device_iot_key);
}

#endif
