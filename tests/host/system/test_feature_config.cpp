/************************ Feature Config File Tests ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers a feature's own /etc config: rendering a record as options, taking the
file's options back into a record, seeding the file from the record store when
it says nothing, and keeping a file that carries a secret unreadable to others.

Author          : Suraj I.
created Date    : 3rd Sep 2026
******************************************************************************/

#include <MountedStack.h>
#include <pditest.h>

#include <helpers/FeatureConfigFiles.h>
#include <service_provider/database/DatabaseServiceProvider.h>

#if defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_WIFI_SERVICE) && defined(ENABLE_WIFI_CONFIG_FILE)

namespace
{

const char *WIFI_CONF_PATH = "/etc/wifi/wifi.conf";

void dropConf(VfsDispatcher *fs)
{
    if (fs->isFileExist(WIFI_CONF_PATH))
    {
        fs->deleteFile(WIFI_CONF_PATH);
    }
}

void writeConf(VfsDispatcher *fs, const char *content)
{
    dropConf(fs);
    if (!fs->isDirExist("/etc/wifi"))
    {
        fs->createDirectory("/etc/wifi");
    }
    fs->createFile(WIFI_CONF_PATH, content);
}

pdiutil::string confValue(const char *key)
{
    pdiutil::string value;
    getConfigValue(WIFI_CONF_PATH, key, value);
    return value;
}

wifi_config_table namedTable(const char *ssid, const char *password)
{
    wifi_config_table table;
    memset(table.sta_ssid, 0, WIFI_CONFIGS_BUF_SIZE);
    memcpy(table.sta_ssid, ssid, strlen(ssid));
    memset(table.sta_password, 0, WIFI_CONFIGS_BUF_SIZE);
    memcpy(table.sta_password, password, strlen(password));
    return table;
}

} // namespace

/* -------------------------------------------------------------- rendering */

TEST(featureconfig, a_record_renders_every_option_the_file_carries)
{
    wifi_config_table table = namedTable("homenet", "s3cret");
    table.sta_local_ip[0] = 10;
    table.sta_local_ip[1] = 151;
    table.sta_local_ip[2] = 111;
    table.sta_local_ip[3] = 28;

    pdiutil::vector<config_kv_t> kvs;
    wifiConfigToKvs(&table, kvs);

    ASSERT_EQ((uint32_t)kvs.size(), 12u);

    pdiutil::string value;
    pdiutil::string key = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_LOCAL_IP);
    bool found = false;
    for (size_t i = 0; i < kvs.size(); i++)
    {
        if (kvs[i].m_key == key)
        {
            value = kvs[i].m_value;
            found = true;
        }
    }

    ASSERT_TRUE(found);
    ASSERT_STREQ(value.c_str(), "10.151.111.28");
}

TEST(featureconfig, an_option_the_file_carries_is_taken_into_the_record)
{
    wifi_config_table table = namedTable("fromdb", "dbpass");

    pdiutil::vector<config_kv_t> kvs;
    config_kv_t kv;
    kv.m_key = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_SSID);
    kv.m_value = "fromfile";
    kvs.push_back(kv);

    ASSERT_TRUE(wifiConfigFromKvs(kvs, &table));
    ASSERT_STREQ(table.sta_ssid, "fromfile");
    ASSERT_STREQ(table.sta_password, "dbpass");
}

TEST(featureconfig, an_option_the_file_does_not_carry_keeps_the_record_value)
{
    wifi_config_table table = namedTable("fromdb", "dbpass");

    pdiutil::vector<config_kv_t> empty;
    ASSERT_FALSE(wifiConfigFromKvs(empty, &table));
    ASSERT_STREQ(table.sta_ssid, "fromdb");
}

TEST(featureconfig, an_option_may_be_cleared_by_giving_it_no_value)
{
    wifi_config_table table = namedTable("fromdb", "dbpass");

    pdiutil::vector<config_kv_t> kvs;
    config_kv_t kv;
    kv.m_key = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_PASSWORD);
    kv.m_value = "";
    kvs.push_back(kv);

    ASSERT_TRUE(wifiConfigFromKvs(kvs, &table));
    ASSERT_STREQ(table.sta_password, "");
}

TEST(featureconfig, an_address_that_does_not_read_keeps_the_record_value)
{
    wifi_config_table table;
    table.sta_gateway[0] = 192;
    table.sta_gateway[1] = 168;
    table.sta_gateway[2] = 1;
    table.sta_gateway[3] = 1;

    pdiutil::vector<config_kv_t> kvs;
    config_kv_t kv;
    kv.m_key = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_GATEWAY);
    kv.m_value = "not.an.address";
    kvs.push_back(kv);

    wifiConfigFromKvs(kvs, &table);

    ASSERT_EQ((uint32_t)table.sta_gateway[0], 192u);
    ASSERT_EQ((uint32_t)table.sta_gateway[3], 1u);
}

/* ----------------------------------------------------------------- gates */

TEST(featureconfig, a_record_defaults_to_both_interfaces_enabled)
{
    wifi_config_table table;
    ASSERT_TRUE(table.sta_enable);
    ASSERT_TRUE(table.ap_enable);
}

TEST(featureconfig, the_gates_round_trip_through_the_file_form)
{
    wifi_config_table table;
    table.sta_enable = false;
    table.ap_enable = true;

    pdiutil::vector<config_kv_t> kvs;
    wifiConfigToKvs(&table, kvs);

    wifi_config_table after;
    ASSERT_TRUE(wifiConfigFromKvs(kvs, &after));
    ASSERT_FALSE(after.sta_enable);
    ASSERT_TRUE(after.ap_enable);
}

TEST(featureconfig, a_gate_the_file_turns_off_is_taken_into_the_record)
{
    wifi_config_table table;

    pdiutil::vector<config_kv_t> kvs;
    config_kv_t kv;
    kv.m_key = CHARPTR_WRAP(WIFI_CONFIG_KEY_AP_ENABLE);
    kv.m_value = "no";
    kvs.push_back(kv);

    ASSERT_TRUE(wifiConfigFromKvs(kvs, &table));
    ASSERT_FALSE(table.ap_enable);
    ASSERT_TRUE(table.sta_enable);
}

TEST(featureconfig, a_gate_that_does_not_read_keeps_the_record_value)
{
    wifi_config_table table;
    table.sta_enable = false;

    pdiutil::vector<config_kv_t> kvs;
    config_kv_t kv;
    kv.m_key = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_ENABLE);
    kv.m_value = "maybe";
    kvs.push_back(kv);

    wifiConfigFromKvs(kvs, &table);
    ASSERT_FALSE(table.sta_enable);
}

TEST(featureconfig, both_gates_may_be_turned_off_together)
{
    wifi_config_table table;

    pdiutil::vector<config_kv_t> kvs;
    config_kv_t sta;
    sta.m_key = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_ENABLE);
    sta.m_value = "no";
    kvs.push_back(sta);
    config_kv_t ap;
    ap.m_key = CHARPTR_WRAP(WIFI_CONFIG_KEY_AP_ENABLE);
    ap.m_value = "no";
    kvs.push_back(ap);

    ASSERT_TRUE(wifiConfigFromKvs(kvs, &table));
    ASSERT_FALSE(table.sta_enable);
    ASSERT_FALSE(table.ap_enable);
}

/* ------------------------------------------------------------------ sync */

TEST(featureconfig, a_seeded_file_carries_its_whole_header_and_every_option)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();
    dropConf(fs);

    wifi_config_table table = namedTable("seedme", "seedpass");
    __database_service.set_wifi_config_table(&table);

    pdiutil::vector<config_kv_t> present;
    ASSERT_TRUE(loadConfigFile(WIFI_CONF_PATH, present));
    ASSERT_EQ((uint32_t)present.size(), 12u);

    pdiutil::string header = CHARPTR_WRAP(WIFI_CONFIG_HEADER);
    int64_t written = fs->getFileSize(WIFI_CONF_PATH);
    ASSERT_TRUE(written > (int64_t)header.size());

    ASSERT_STREQ(confValue(WIFI_CONFIG_KEY_AP_SUBNET).c_str(), "255.255.255.0");

    dropConf(fs);
}

TEST(featureconfig, a_missing_file_is_seeded_from_the_record_store)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();
    dropConf(fs);

    wifi_config_table table = namedTable("seedme", "seedpass");
    __database_service.set_wifi_config_table(&table);

    ASSERT_TRUE(fs->isFileExist(WIFI_CONF_PATH));
    ASSERT_STREQ(confValue(WIFI_CONFIG_KEY_STA_SSID).c_str(), "seedme");

    dropConf(fs);
}

TEST(featureconfig, a_sync_leaves_the_option_the_file_already_carried)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();

    writeConf(fs, "sta_ssid fromfile\r\n");
    syncWifiConfigFile();

    ASSERT_STREQ(confValue(WIFI_CONFIG_KEY_STA_SSID).c_str(), "fromfile");

    dropConf(fs);
}

TEST(featureconfig, an_option_the_file_lacks_is_appended_by_a_sync)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();

    writeConf(fs, "sta_ssid fromfile\r\n");
    syncWifiConfigFile();

    ASSERT_STREQ(confValue(WIFI_CONFIG_KEY_STA_SSID).c_str(), "fromfile");
    ASSERT_FALSE(confValue(WIFI_CONFIG_KEY_STA_PASSWORD).empty());
    ASSERT_FALSE(confValue(WIFI_CONFIG_KEY_AP_SUBNET).empty());

    dropConf(fs);
}

TEST(featureconfig, a_key_the_feature_does_not_own_survives_a_sync)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();

    writeConf(fs, "enabled no\r\nsta_ssid fromfile\r\n");
    syncWifiConfigFile();

    ASSERT_STREQ(confValue("enabled").c_str(), "no");
    ASSERT_STREQ(confValue(WIFI_CONFIG_KEY_STA_SSID).c_str(), "fromfile");

    dropConf(fs);
}

/* ----------------------------------------------------------- permissions */

TEST(featureconfig, a_seeded_config_carrying_a_secret_is_not_world_readable)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();
    dropConf(fs);

    wifi_config_table table = namedTable("seedme", "seedpass");
    __database_service.set_wifi_config_table(&table);

    file_info_t meta;
    ASSERT_EQ(fs->getFileMeta(WIFI_CONF_PATH, meta), (pdi_err_t)PDI_OK);
    ASSERT_EQ((uint32_t)meta.m_perms, 0600u);
    ASSERT_EQ((uint32_t)meta.m_uid, 0u);

    dropConf(fs);
}

TEST(featureconfig, a_config_another_writer_created_open_is_tightened_on_the_next_save)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();
    writeConf(fs, "enabled yes\r\n");

    file_info_t before;
    ASSERT_EQ(fs->getFileMeta(WIFI_CONF_PATH, before), (pdi_err_t)PDI_OK);
    ASSERT_TRUE(0600u != (uint32_t)before.m_perms);

    wifi_config_table table = namedTable("seedme", "seedpass");
    __database_service.set_wifi_config_table(&table);

    file_info_t after;
    ASSERT_EQ(fs->getFileMeta(WIFI_CONF_PATH, after), (pdi_err_t)PDI_OK);
    ASSERT_EQ((uint32_t)after.m_perms, 0600u);
    ASSERT_EQ((uint32_t)after.m_uid, 0u);

    dropConf(fs);
}

TEST(featureconfig, rewriting_one_option_keeps_the_file_unreadable_to_others)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();
    dropConf(fs);

    wifi_config_table table = namedTable("seedme", "seedpass");
    __database_service.set_wifi_config_table(&table);

    wifi_config_table changed = namedTable("changed", "seedpass");
    __database_service.set_wifi_config_table(&changed);

    ASSERT_STREQ(confValue(WIFI_CONFIG_KEY_STA_SSID).c_str(), "changed");

    file_info_t meta;
    ASSERT_EQ(fs->getFileMeta(WIFI_CONF_PATH, meta), (pdi_err_t)PDI_OK);
    ASSERT_EQ((uint32_t)meta.m_perms, 0600u);

    dropConf(fs);
}

#endif

#if defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_MQTT_SERVICE) && defined(ENABLE_MQTT_CONFIG_FILE)

namespace
{

const char *MQTT_CONF_PATH = "/etc/mqtt/mqtt.conf";

void dropMqttConf(VfsDispatcher *fs)
{
    if (fs->isFileExist(MQTT_CONF_PATH))
    {
        fs->deleteFile(MQTT_CONF_PATH);
    }
}

void writeMqttConf(VfsDispatcher *fs, const char *content)
{
    dropMqttConf(fs);
    if (!fs->isDirExist("/etc/mqtt"))
    {
        fs->createDirectory("/etc/mqtt");
    }
    fs->createFile(MQTT_CONF_PATH, content);
}

pdiutil::string mqttConfValue(const char *key)
{
    pdiutil::string value;
    getConfigValue(MQTT_CONF_PATH, key, value);
    return value;
}

config_kv_t mqttKv(const char *key, const char *value)
{
    config_kv_t kv;
    kv.m_key = key;
    kv.m_value = value;
    return kv;
}

mqtt_general_config_table brokerTable(const char *host, const char *password)
{
    mqtt_general_config_table table;
    memcpy(table.host, host, strlen(host));
    memcpy(table.password, password, strlen(password));
    return table;
}

} // namespace

/* --------------------------------------------------------- mqtt rendering */

TEST(featureconfig, an_mqtt_record_renders_every_option_the_file_carries)
{
    mqtt_general_config_table general = brokerTable("broker.local", "s3cret");
    mqtt_lwt_config_table lwt;

    pdiutil::vector<config_kv_t> kvs;
    mqttConfigToKvs(&general, &lwt, kvs);

    ASSERT_EQ((uint32_t)kvs.size(), 12u);

    pdiutil::string port;
    pdiutil::string key = CHARPTR_WRAP(MQTT_CONFIG_KEY_PORT);
    ASSERT_TRUE(findConfigValue(kvs, key, port));
    ASSERT_STREQ(port.c_str(), "1883");

    pdiutil::string security;
    pdiutil::string security_key = CHARPTR_WRAP(MQTT_CONFIG_KEY_SECURITY);
    ASSERT_TRUE(findConfigValue(kvs, security_key, security));
    ASSERT_STREQ(security.c_str(), "no");
}

TEST(featureconfig, the_mqtt_config_carries_no_publish_or_subscribe_topic)
{
    mqtt_general_config_table general;
    mqtt_lwt_config_table lwt;

    pdiutil::vector<config_kv_t> kvs;
    mqttConfigToKvs(&general, &lwt, kvs);

    for (size_t i = 0; i < kvs.size(); i++)
    {
        ASSERT_EQ(__strstr(kvs[i].m_key.c_str(), "publish"), -1);
        ASSERT_EQ(__strstr(kvs[i].m_key.c_str(), "subscribe"), -1);
    }
}

TEST(featureconfig, an_mqtt_option_the_file_carries_is_taken_into_the_record)
{
    mqtt_general_config_table general = brokerTable("fromdb", "dbpass");
    mqtt_lwt_config_table lwt;

    pdiutil::vector<config_kv_t> kvs;
    kvs.push_back(mqttKv(MQTT_CONFIG_KEY_HOST, "fromfile"));
    kvs.push_back(mqttKv(MQTT_CONFIG_KEY_PORT, "8883"));

    ASSERT_TRUE(mqttConfigFromKvs(kvs, &general, &lwt));
    ASSERT_STREQ(general.host, "fromfile");
    ASSERT_EQ((uint32_t)general.port, 8883u);
    ASSERT_STREQ(general.password, "dbpass");
}

TEST(featureconfig, one_file_feeds_both_the_broker_and_the_last_will_record)
{
    mqtt_general_config_table general;
    mqtt_lwt_config_table lwt;

    pdiutil::vector<config_kv_t> kvs;
    kvs.push_back(mqttKv(MQTT_CONFIG_KEY_HOST, "broker.local"));
    kvs.push_back(mqttKv(MQTT_CONFIG_KEY_WILL_TOPIC, "pdi/status"));
    kvs.push_back(mqttKv(MQTT_CONFIG_KEY_WILL_QOS, "2"));

    ASSERT_TRUE(mqttConfigFromKvs(kvs, &general, &lwt));
    ASSERT_STREQ(general.host, "broker.local");
    ASSERT_STREQ(lwt.will_topic, "pdi/status");
    ASSERT_EQ((uint32_t)lwt.will_qos, 2u);
}

/* ----------------------------------------------------------- mqtt numbers */

TEST(featureconfig, a_number_that_does_not_read_keeps_the_record_value)
{
    mqtt_general_config_table general;
    mqtt_lwt_config_table lwt;
    general.port = 1883;

    pdiutil::vector<config_kv_t> kvs;
    kvs.push_back(mqttKv(MQTT_CONFIG_KEY_PORT, "eighteen eighty three"));

    ASSERT_FALSE(mqttConfigFromKvs(kvs, &general, &lwt));
    ASSERT_EQ((uint32_t)general.port, 1883u);
}

TEST(featureconfig, a_number_beyond_what_the_field_holds_keeps_the_record_value)
{
    mqtt_general_config_table general;
    mqtt_lwt_config_table lwt;
    general.port = 1883;
    lwt.will_qos = 1;

    pdiutil::vector<config_kv_t> kvs;
    kvs.push_back(mqttKv(MQTT_CONFIG_KEY_PORT, "70000"));
    kvs.push_back(mqttKv(MQTT_CONFIG_KEY_WILL_QOS, "3"));

    ASSERT_FALSE(mqttConfigFromKvs(kvs, &general, &lwt));
    ASSERT_EQ((uint32_t)general.port, 1883u);
    ASSERT_EQ((uint32_t)lwt.will_qos, 1u);
}

TEST(featureconfig, a_flag_kept_as_a_byte_round_trips_through_the_file_form)
{
    mqtt_general_config_table general;
    mqtt_lwt_config_table lwt;
    general.clean_session = 1;
    lwt.will_retain = 0;

    pdiutil::vector<config_kv_t> kvs;
    mqttConfigToKvs(&general, &lwt, kvs);

    pdiutil::string value;
    pdiutil::string key = CHARPTR_WRAP(MQTT_CONFIG_KEY_CLEAN_SESSION);
    ASSERT_TRUE(findConfigValue(kvs, key, value));
    ASSERT_STREQ(value.c_str(), "yes");

    pdiutil::vector<config_kv_t> off;
    off.push_back(mqttKv(MQTT_CONFIG_KEY_CLEAN_SESSION, "no"));
    off.push_back(mqttKv(MQTT_CONFIG_KEY_WILL_RETAIN, "yes"));

    ASSERT_TRUE(mqttConfigFromKvs(off, &general, &lwt));
    ASSERT_EQ((uint32_t)general.clean_session, 0u);
    ASSERT_EQ((uint32_t)lwt.will_retain, 1u);
}

TEST(featureconfig, the_broker_security_flag_round_trips_through_the_file_form)
{
    mqtt_general_config_table general;
    mqtt_lwt_config_table lwt;
    general.security = 1;

    pdiutil::vector<config_kv_t> kvs;
    mqttConfigToKvs(&general, &lwt, kvs);

    pdiutil::string value;
    pdiutil::string key = CHARPTR_WRAP(MQTT_CONFIG_KEY_SECURITY);
    ASSERT_TRUE(findConfigValue(kvs, key, value));
    ASSERT_STREQ(value.c_str(), "yes");

    pdiutil::vector<config_kv_t> off;
    off.push_back(mqttKv(MQTT_CONFIG_KEY_SECURITY, "no"));

    ASSERT_TRUE(mqttConfigFromKvs(off, &general, &lwt));
    ASSERT_EQ((uint32_t)general.security, 0u);
}

TEST(featureconfig, a_file_with_no_security_key_leaves_the_broker_flag_alone)
{
    mqtt_general_config_table general;
    mqtt_lwt_config_table lwt;
    general.security = 1;

    pdiutil::vector<config_kv_t> kvs;
    kvs.push_back(mqttKv(MQTT_CONFIG_KEY_HOST, "broker.example.com"));

    ASSERT_TRUE(mqttConfigFromKvs(kvs, &general, &lwt));
    ASSERT_EQ((uint32_t)general.security, 1u);
}

/* -------------------------------------------------------------- mqtt sync */

TEST(featureconfig, a_missing_mqtt_file_is_seeded_from_both_records)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();
    dropMqttConf(fs);

    mqtt_general_config_table general = brokerTable("seedbroker", "seedpass");
    ASSERT_TRUE(__database_service.set_mqtt_general_config_table(&general));

    ASSERT_TRUE(fs->isFileExist(MQTT_CONF_PATH));
    ASSERT_STREQ(mqttConfValue(MQTT_CONFIG_KEY_HOST).c_str(), "seedbroker");
    ASSERT_STREQ(mqttConfValue(MQTT_CONFIG_KEY_KEEPALIVE).c_str(), "30");
    ASSERT_FALSE(mqttConfValue(MQTT_CONFIG_KEY_WILL_RETAIN).empty());

    dropMqttConf(fs);
}

TEST(featureconfig, saving_one_mqtt_record_keeps_the_other_records_options)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();
    dropMqttConf(fs);

    mqtt_lwt_config_table lwt;
    memcpy(lwt.will_topic, "pdi/gone", strlen("pdi/gone"));
    __database_service.set_mqtt_lwt_config_table(&lwt);

    mqtt_general_config_table general = brokerTable("seedbroker", "seedpass");
    __database_service.set_mqtt_general_config_table(&general);

    ASSERT_STREQ(mqttConfValue(MQTT_CONFIG_KEY_HOST).c_str(), "seedbroker");
    ASSERT_STREQ(mqttConfValue(MQTT_CONFIG_KEY_WILL_TOPIC).c_str(), "pdi/gone");

    dropMqttConf(fs);
}

TEST(featureconfig, an_mqtt_sync_leaves_the_option_the_file_already_carried)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();

    writeMqttConf(fs, "host fromfile\r\nport 8883\r\n");
    syncMqttConfigFile();

    ASSERT_STREQ(mqttConfValue(MQTT_CONFIG_KEY_HOST).c_str(), "fromfile");
    ASSERT_STREQ(mqttConfValue(MQTT_CONFIG_KEY_PORT).c_str(), "8883");
    ASSERT_FALSE(mqttConfValue(MQTT_CONFIG_KEY_WILL_QOS).empty());

    mqtt_general_config_table general;
    __database_service.get_mqtt_general_config_table(&general);
    ASSERT_EQ((uint32_t)general.port, 8883u);

    dropMqttConf(fs);
}

/* ------------------------------------------------------- mqtt permissions */

TEST(featureconfig, a_seeded_mqtt_config_carrying_a_secret_is_not_world_readable)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();
    dropMqttConf(fs);

    mqtt_general_config_table general = brokerTable("seedbroker", "seedpass");
    __database_service.set_mqtt_general_config_table(&general);

    file_info_t meta;
    ASSERT_EQ(fs->getFileMeta(MQTT_CONF_PATH, meta), (pdi_err_t)PDI_OK);
    ASSERT_EQ((uint32_t)meta.m_perms, 0600u);
    ASSERT_EQ((uint32_t)meta.m_uid, 0u);

    dropMqttConf(fs);
}

#endif

#if defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_OTA_SERVICE) && defined(ENABLE_OTA_CONFIG_FILE)

namespace
{

const char *OTA_CONF_PATH = "/etc/ota/ota.conf";

void dropOtaConf(VfsDispatcher *fs)
{
    if (fs->isFileExist(OTA_CONF_PATH))
    {
        fs->deleteFile(OTA_CONF_PATH);
    }
}

void writeOtaConf(VfsDispatcher *fs, const char *content)
{
    dropOtaConf(fs);
    if (!fs->isDirExist("/etc/ota"))
    {
        fs->createDirectory("/etc/ota");
    }
    fs->createFile(OTA_CONF_PATH, content);
}

pdiutil::string otaConfValue(const char *key)
{
    pdiutil::string value;
    getConfigValue(OTA_CONF_PATH, key, value);
    return value;
}

ota_config_table serverTable(const char *host, uint16_t port)
{
    ota_config_table table;
    memcpy(table.ota_host, host, strlen(host));
    table.ota_port = port;
    return table;
}

} // namespace

TEST(featureconfig, an_ota_record_renders_every_option_the_file_carries)
{
    ota_config_table table = serverTable("updates.local", 8080);

    pdiutil::vector<config_kv_t> kvs;
    otaConfigToKvs(&table, kvs);

    ASSERT_EQ((uint32_t)kvs.size(), 2u);

    pdiutil::string value;
    pdiutil::string key = CHARPTR_WRAP(OTA_CONFIG_KEY_PORT);
    ASSERT_TRUE(findConfigValue(kvs, key, value));
    ASSERT_STREQ(value.c_str(), "8080");
}

TEST(featureconfig, an_ota_option_the_file_carries_is_taken_into_the_record)
{
    ota_config_table table = serverTable("fromdb", 80);

    pdiutil::vector<config_kv_t> kvs;
    config_kv_t kv;
    kv.m_key = OTA_CONFIG_KEY_HOST;
    kv.m_value = "fromfile";
    kvs.push_back(kv);

    ASSERT_TRUE(otaConfigFromKvs(kvs, &table));
    ASSERT_STREQ(table.ota_host, "fromfile");
    ASSERT_EQ((uint32_t)table.ota_port, 80u);
}

TEST(featureconfig, an_ota_port_that_does_not_read_keeps_the_record_value)
{
    ota_config_table table = serverTable("updates.local", 80);

    pdiutil::vector<config_kv_t> kvs;
    config_kv_t kv;
    kv.m_key = OTA_CONFIG_KEY_PORT;
    kv.m_value = "http";
    kvs.push_back(kv);

    ASSERT_FALSE(otaConfigFromKvs(kvs, &table));
    ASSERT_EQ((uint32_t)table.ota_port, 80u);
}

TEST(featureconfig, a_missing_ota_file_is_seeded_from_the_record_store)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();
    dropOtaConf(fs);

    ota_config_table table = serverTable("updates.local", 8080);
    ASSERT_TRUE(__database_service.set_ota_config_table(&table));

    ASSERT_TRUE(fs->isFileExist(OTA_CONF_PATH));
    ASSERT_STREQ(otaConfValue(OTA_CONFIG_KEY_HOST).c_str(), "updates.local");
    ASSERT_STREQ(otaConfValue(OTA_CONFIG_KEY_PORT).c_str(), "8080");

    dropOtaConf(fs);
}

TEST(featureconfig, an_ota_sync_leaves_the_option_the_file_already_carried)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();

    writeOtaConf(fs, "host fromfile\r\n");
    syncOtaConfigFile();

    ASSERT_STREQ(otaConfValue(OTA_CONFIG_KEY_HOST).c_str(), "fromfile");
    ASSERT_FALSE(otaConfValue(OTA_CONFIG_KEY_PORT).empty());

    ota_config_table table;
    __database_service.get_ota_config_table(&table);
    ASSERT_STREQ(table.ota_host, "fromfile");

    dropOtaConf(fs);
}

TEST(featureconfig, a_seeded_ota_config_is_owned_by_root)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();
    dropOtaConf(fs);

    ota_config_table table = serverTable("updates.local", 8080);
    __database_service.set_ota_config_table(&table);

    file_info_t meta;
    ASSERT_EQ(fs->getFileMeta(OTA_CONF_PATH, meta), (pdi_err_t)PDI_OK);
    ASSERT_EQ((uint32_t)meta.m_perms, 0600u);
    ASSERT_EQ((uint32_t)meta.m_uid, 0u);

    dropOtaConf(fs);
}

#endif

#if defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_EMAIL_SERVICE) && defined(ENABLE_EMAIL_CONFIG_FILE)

namespace
{

const char *EMAIL_CONF_PATH = "/etc/email/email.conf";

void dropEmailConf(VfsDispatcher *fs)
{
    if (fs->isFileExist(EMAIL_CONF_PATH))
    {
        fs->deleteFile(EMAIL_CONF_PATH);
    }
}

void writeEmailConf(VfsDispatcher *fs, const char *content)
{
    dropEmailConf(fs);
    if (!fs->isDirExist("/etc/email"))
    {
        fs->createDirectory("/etc/email");
    }
    fs->createFile(EMAIL_CONF_PATH, content);
}

pdiutil::string emailConfValue(const char *key)
{
    pdiutil::string value;
    getConfigValue(EMAIL_CONF_PATH, key, value);
    return value;
}

email_config_table mailboxTable(const char *host, const char *password)
{
    email_config_table table;
    memset(table.mail_host, 0, DEFAULT_MAIL_HOST_MAX_SIZE);
    memcpy(table.mail_host, host, strlen(host));
    memset(table.mail_password, 0, DEFAULT_MAIL_PASSWORD_MAX_SIZE);
    memcpy(table.mail_password, password, strlen(password));
    return table;
}

} // namespace

TEST(featureconfig, an_email_record_renders_every_option_the_file_carries)
{
    email_config_table table = mailboxTable("smtp.local", "s3cret");

    pdiutil::vector<config_kv_t> kvs;
    emailConfigToKvs(&table, kvs);

    ASSERT_EQ((uint32_t)kvs.size(), 9u);

    pdiutil::string value;
    pdiutil::string key = CHARPTR_WRAP(EMAIL_CONFIG_KEY_HOST);
    ASSERT_TRUE(findConfigValue(kvs, key, value));
    ASSERT_STREQ(value.c_str(), "smtp.local");
}

TEST(featureconfig, the_email_config_carries_no_send_frequency)
{
    email_config_table table;

    pdiutil::vector<config_kv_t> kvs;
    emailConfigToKvs(&table, kvs);

    for (size_t i = 0; i < kvs.size(); i++)
    {
        ASSERT_EQ(__strstr(kvs[i].m_key.c_str(), "frequency"), -1);
    }
}

TEST(featureconfig, an_email_option_the_file_carries_is_taken_into_the_record)
{
    email_config_table table = mailboxTable("fromdb", "dbpass");

    pdiutil::vector<config_kv_t> kvs;
    config_kv_t kv;
    kv.m_key = EMAIL_CONFIG_KEY_HOST;
    kv.m_value = "smtp.fromfile";
    kvs.push_back(kv);

    ASSERT_TRUE(emailConfigFromKvs(kvs, &table));
    ASSERT_STREQ(table.mail_host, "smtp.fromfile");
    ASSERT_STREQ(table.mail_password, "dbpass");
}

TEST(featureconfig, an_email_port_that_does_not_read_keeps_the_record_value)
{
    email_config_table table;
    table.mail_port = 2525;

    pdiutil::vector<config_kv_t> kvs;
    config_kv_t kv;
    kv.m_key = EMAIL_CONFIG_KEY_PORT;
    kv.m_value = "smtp";
    kvs.push_back(kv);

    ASSERT_FALSE(emailConfigFromKvs(kvs, &table));
    ASSERT_EQ((uint32_t)table.mail_port, 2525u);
}

TEST(featureconfig, a_missing_email_file_is_seeded_from_the_record_store)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();
    dropEmailConf(fs);

    email_config_table table = mailboxTable("smtp.local", "seedpass");
    ASSERT_TRUE(__database_service.set_email_config_table(&table));

    ASSERT_TRUE(fs->isFileExist(EMAIL_CONF_PATH));
    ASSERT_STREQ(emailConfValue(EMAIL_CONFIG_KEY_HOST).c_str(), "smtp.local");
    ASSERT_FALSE(emailConfValue(EMAIL_CONFIG_KEY_SUBJECT).empty());

    dropEmailConf(fs);
}

TEST(featureconfig, an_email_sync_leaves_the_option_the_file_already_carried)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();

    writeEmailConf(fs, "host smtp.fromfile\r\nport 587\r\n");
    syncEmailConfigFile();

    ASSERT_STREQ(emailConfValue(EMAIL_CONFIG_KEY_HOST).c_str(), "smtp.fromfile");
    ASSERT_STREQ(emailConfValue(EMAIL_CONFIG_KEY_PORT).c_str(), "587");

    email_config_table table;
    __database_service.get_email_config_table(&table);
    ASSERT_EQ((uint32_t)table.mail_port, 587u);

    dropEmailConf(fs);
}

TEST(featureconfig, a_seeded_email_config_carrying_a_secret_is_not_world_readable)
{
    VfsDispatcher *fs = pditest::mountedVfs();
    pditest::mountedDb();
    dropEmailConf(fs);

    email_config_table table = mailboxTable("smtp.local", "seedpass");
    __database_service.set_email_config_table(&table);

    file_info_t meta;
    ASSERT_EQ(fs->getFileMeta(EMAIL_CONF_PATH, meta), (pdi_err_t)PDI_OK);
    ASSERT_EQ((uint32_t)meta.m_perms, 0600u);
    ASSERT_EQ((uint32_t)meta.m_uid, 0u);

    dropEmailConf(fs);
}

#endif
