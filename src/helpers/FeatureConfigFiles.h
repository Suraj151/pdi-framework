/*************************** Feature config files *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 3rd Sep 2026
******************************************************************************/

#ifndef _FEATURE_CONFIG_FILES_H_
#define _FEATURE_CONFIG_FILES_H_

#include "ConfigHelper.h"

#if defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_WIFI_SERVICE) && defined(ENABLE_WIFI_CONFIG_FILE)

#define WIFI_CONFIG_FEATURE_NAME "wifi"
#define WIFI_CONFIG_KEY_STA_SSID "sta_ssid"
#define WIFI_CONFIG_KEY_STA_PASSWORD "sta_password"
#define WIFI_CONFIG_KEY_AP_SSID "ap_ssid"
#define WIFI_CONFIG_KEY_AP_PASSWORD "ap_password"
#define WIFI_CONFIG_KEY_STA_LOCAL_IP "sta_local_ip"
#define WIFI_CONFIG_KEY_STA_GATEWAY "sta_gateway"
#define WIFI_CONFIG_KEY_STA_SUBNET "sta_subnet"
#define WIFI_CONFIG_KEY_AP_LOCAL_IP "ap_local_ip"
#define WIFI_CONFIG_KEY_AP_GATEWAY "ap_gateway"
#define WIFI_CONFIG_KEY_AP_SUBNET "ap_subnet"
#define WIFI_CONFIG_KEY_STA_ENABLE "sta_enable"
#define WIFI_CONFIG_KEY_AP_ENABLE "ap_enable"

#define WIFI_CONFIG_HEADER \
    "# PDI wifi configuration" TERMINAL_NEW_LINE \
    "# addresses are dotted quads, an unreadable one keeps the built in value" TERMINAL_NEW_LINE \
    "# sta_enable and ap_enable gate whether the station and access point come up" TERMINAL_NEW_LINE

/**
 * Render a wifi config record as the key/value pairs its config file carries.
 */
void wifiConfigToKvs(const wifi_config_table *_table, pdiutil::vector<config_kv_t> &_out);

/**
 * Take into a wifi config record whatever keys the config file supplied, so a
 * key the file does not carry keeps the value it arrived with.
 */
bool wifiConfigFromKvs(const pdiutil::vector<config_kv_t> &_kvs, wifi_config_table *_table);

/**
 * Write a wifi config record out to its config file, touching only the options
 * whose value actually changed so comments and ordering survive.
 */
bool writeWifiConfigFile(const wifi_config_table *_table);

/**
 * Bring the wifi config file and the record store into agreement, the file
 * winning where it carries a key and being seeded from the record where not.
 */
bool syncWifiConfigFile();

#endif

#if defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_MQTT_SERVICE) && defined(ENABLE_MQTT_CONFIG_FILE)

#define MQTT_CONFIG_FEATURE_NAME "mqtt"
#define MQTT_CONFIG_KEY_HOST "host"
#define MQTT_CONFIG_KEY_PORT "port"
#define MQTT_CONFIG_KEY_SECURITY "security"
#define MQTT_CONFIG_KEY_CLIENT_ID "client_id"
#define MQTT_CONFIG_KEY_USERNAME "username"
#define MQTT_CONFIG_KEY_PASSWORD "password"
#define MQTT_CONFIG_KEY_KEEPALIVE "keepalive"
#define MQTT_CONFIG_KEY_CLEAN_SESSION "clean_session"
#define MQTT_CONFIG_KEY_WILL_TOPIC "will_topic"
#define MQTT_CONFIG_KEY_WILL_MESSAGE "will_message"
#define MQTT_CONFIG_KEY_WILL_QOS "will_qos"
#define MQTT_CONFIG_KEY_WILL_RETAIN "will_retain"

#define MQTT_CONFIG_HEADER \
    "# PDI mqtt configuration" TERMINAL_NEW_LINE \
    "# keepalive is in seconds, will_qos runs from 0 to 2" TERMINAL_NEW_LINE \
    "# the publish and subscribe topics are runtime state, not settings" TERMINAL_NEW_LINE

/**
 * Render the mqtt broker and last will records as the key/value pairs their
 * shared config file carries.
 */
void mqttConfigToKvs(const mqtt_general_config_table *_general, const mqtt_lwt_config_table *_lwt, pdiutil::vector<config_kv_t> &_out);

/**
 * Take into the mqtt records whatever keys the config file supplied, so a key
 * the file does not carry keeps the value it arrived with.
 */
bool mqttConfigFromKvs(const pdiutil::vector<config_kv_t> &_kvs, mqtt_general_config_table *_general, mqtt_lwt_config_table *_lwt);

/**
 * Write both mqtt records out to their config file. The records are read back
 * from the store so a save of one of them cannot drop the other's options.
 */
bool writeMqttConfigFile();

/**
 * Bring the mqtt config file and the record store into agreement, the file
 * winning where it carries a key and being seeded from the records where not.
 */
bool syncMqttConfigFile();

#endif

#if defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_OTA_SERVICE) && defined(ENABLE_OTA_CONFIG_FILE)

#define OTA_CONFIG_FEATURE_NAME "ota"
#define OTA_CONFIG_KEY_HOST "host"
#define OTA_CONFIG_KEY_PORT "port"

#define OTA_CONFIG_HEADER \
    "# PDI ota configuration" TERMINAL_NEW_LINE \
    "# host is the update server this device asks for a new image" TERMINAL_NEW_LINE

/**
 * Render an ota config record as the key/value pairs its config file carries.
 */
void otaConfigToKvs(const ota_config_table *_table, pdiutil::vector<config_kv_t> &_out);

/**
 * Take into an ota config record whatever keys the config file supplied, so a
 * key the file does not carry keeps the value it arrived with.
 */
bool otaConfigFromKvs(const pdiutil::vector<config_kv_t> &_kvs, ota_config_table *_table);

/**
 * Write an ota config record out to its config file, touching only the options
 * whose value actually changed so comments and ordering survive.
 */
bool writeOtaConfigFile(const ota_config_table *_table);

/**
 * Bring the ota config file and the record store into agreement, the file
 * winning where it carries a key and being seeded from the record where not.
 */
bool syncOtaConfigFile();

#endif

#if defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_EMAIL_SERVICE) && defined(ENABLE_EMAIL_CONFIG_FILE)

#define EMAIL_CONFIG_FEATURE_NAME "email"
#define EMAIL_CONFIG_KEY_SENDING_DOMAIN "sending_domain"
#define EMAIL_CONFIG_KEY_HOST "host"
#define EMAIL_CONFIG_KEY_PORT "port"
#define EMAIL_CONFIG_KEY_USERNAME "username"
#define EMAIL_CONFIG_KEY_PASSWORD "password"
#define EMAIL_CONFIG_KEY_FROM "from"
#define EMAIL_CONFIG_KEY_FROM_NAME "from_name"
#define EMAIL_CONFIG_KEY_TO "to"
#define EMAIL_CONFIG_KEY_SUBJECT "subject"

#define EMAIL_CONFIG_HEADER \
    "# PDI email configuration" TERMINAL_NEW_LINE \
    "# host and port are the smtp server this device sends through" TERMINAL_NEW_LINE

/**
 * Render an email config record as the key/value pairs its config file carries.
 */
void emailConfigToKvs(const email_config_table *_table, pdiutil::vector<config_kv_t> &_out);

/**
 * Take into an email config record whatever keys the config file supplied, so a
 * key the file does not carry keeps the value it arrived with.
 */
bool emailConfigFromKvs(const pdiutil::vector<config_kv_t> &_kvs, email_config_table *_table);

/**
 * Write an email config record out to its config file, touching only the
 * options whose value actually changed so comments and ordering survive.
 */
bool writeEmailConfigFile(const email_config_table *_table);

/**
 * Bring the email config file and the record store into agreement, the file
 * winning where it carries a key and being seeded from the record where not.
 */
bool syncEmailConfigFile();

#endif

#endif
