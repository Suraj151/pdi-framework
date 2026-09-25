/*************************** Feature config files *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 3rd Sep 2026
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_STORAGE_SERVICE)

#include "FeatureConfigFiles.h"
#include <service_provider/database/DatabaseServiceProvider.h>

#if defined(ENABLE_WIFI_SERVICE) && defined(ENABLE_WIFI_CONFIG_FILE)

/**
 * Render a wifi config record as the key/value pairs its config file carries.
 */
void wifiConfigToKvs(const wifi_config_table *_table, pdiutil::vector<config_kv_t> &_out)
{
  _out.clear();

  if (nullptr == _table)
  {
    return;
  }

  pdiutil::string key_sta_ssid = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_SSID);
  pdiutil::string key_sta_password = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_PASSWORD);
  pdiutil::string key_ap_ssid = CHARPTR_WRAP(WIFI_CONFIG_KEY_AP_SSID);
  pdiutil::string key_ap_password = CHARPTR_WRAP(WIFI_CONFIG_KEY_AP_PASSWORD);
  pdiutil::string key_sta_local_ip = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_LOCAL_IP);
  pdiutil::string key_sta_gateway = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_GATEWAY);
  pdiutil::string key_sta_subnet = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_SUBNET);
  pdiutil::string key_ap_local_ip = CHARPTR_WRAP(WIFI_CONFIG_KEY_AP_LOCAL_IP);
  pdiutil::string key_ap_gateway = CHARPTR_WRAP(WIFI_CONFIG_KEY_AP_GATEWAY);
  pdiutil::string key_ap_subnet = CHARPTR_WRAP(WIFI_CONFIG_KEY_AP_SUBNET);
  pdiutil::string key_sta_enable = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_ENABLE);
  pdiutil::string key_ap_enable = CHARPTR_WRAP(WIFI_CONFIG_KEY_AP_ENABLE);

  appendConfigValue(_out, key_sta_enable, configBoolAsValue(_table->sta_enable));
  appendConfigValue(_out, key_ap_enable, configBoolAsValue(_table->ap_enable));

  appendConfigValue(_out, key_sta_ssid, pdiutil::string(_table->sta_ssid));
  appendConfigValue(_out, key_sta_password, pdiutil::string(_table->sta_password));
  appendConfigValue(_out, key_ap_ssid, pdiutil::string(_table->ap_ssid));
  appendConfigValue(_out, key_ap_password, pdiutil::string(_table->ap_password));

  appendConfigValue(_out, key_sta_local_ip, configAddressAsValue(_table->sta_local_ip));
  appendConfigValue(_out, key_sta_gateway, configAddressAsValue(_table->sta_gateway));
  appendConfigValue(_out, key_sta_subnet, configAddressAsValue(_table->sta_subnet));

  appendConfigValue(_out, key_ap_local_ip, configAddressAsValue(_table->ap_local_ip));
  appendConfigValue(_out, key_ap_gateway, configAddressAsValue(_table->ap_gateway));
  appendConfigValue(_out, key_ap_subnet, configAddressAsValue(_table->ap_subnet));
}

/**
 * Take into a wifi config record whatever keys the config file supplied, so a
 * key the file does not carry keeps the value it arrived with.
 */
bool wifiConfigFromKvs(const pdiutil::vector<config_kv_t> &_kvs, wifi_config_table *_table)
{
  if (nullptr == _table)
  {
    return false;
  }

  pdiutil::string key_sta_ssid = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_SSID);
  pdiutil::string key_sta_password = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_PASSWORD);
  pdiutil::string key_ap_ssid = CHARPTR_WRAP(WIFI_CONFIG_KEY_AP_SSID);
  pdiutil::string key_ap_password = CHARPTR_WRAP(WIFI_CONFIG_KEY_AP_PASSWORD);
  pdiutil::string key_sta_local_ip = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_LOCAL_IP);
  pdiutil::string key_sta_gateway = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_GATEWAY);
  pdiutil::string key_sta_subnet = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_SUBNET);
  pdiutil::string key_ap_local_ip = CHARPTR_WRAP(WIFI_CONFIG_KEY_AP_LOCAL_IP);
  pdiutil::string key_ap_gateway = CHARPTR_WRAP(WIFI_CONFIG_KEY_AP_GATEWAY);
  pdiutil::string key_ap_subnet = CHARPTR_WRAP(WIFI_CONFIG_KEY_AP_SUBNET);
  pdiutil::string key_sta_enable = CHARPTR_WRAP(WIFI_CONFIG_KEY_STA_ENABLE);
  pdiutil::string key_ap_enable = CHARPTR_WRAP(WIFI_CONFIG_KEY_AP_ENABLE);

  bool applied = false;

  applied |= takeConfigBool(_kvs, key_sta_enable, &_table->sta_enable);
  applied |= takeConfigBool(_kvs, key_ap_enable, &_table->ap_enable);

  applied |= takeConfigText(_kvs, key_sta_ssid, _table->sta_ssid, WIFI_CONFIGS_BUF_SIZE);
  applied |= takeConfigText(_kvs, key_sta_password, _table->sta_password, WIFI_CONFIGS_BUF_SIZE);
  applied |= takeConfigText(_kvs, key_ap_ssid, _table->ap_ssid, WIFI_CONFIGS_BUF_SIZE);
  applied |= takeConfigText(_kvs, key_ap_password, _table->ap_password, WIFI_CONFIGS_BUF_SIZE);

  applied |= takeConfigAddress(_kvs, key_sta_local_ip, _table->sta_local_ip);
  applied |= takeConfigAddress(_kvs, key_sta_gateway, _table->sta_gateway);
  applied |= takeConfigAddress(_kvs, key_sta_subnet, _table->sta_subnet);

  applied |= takeConfigAddress(_kvs, key_ap_local_ip, _table->ap_local_ip);
  applied |= takeConfigAddress(_kvs, key_ap_gateway, _table->ap_gateway);
  applied |= takeConfigAddress(_kvs, key_ap_subnet, _table->ap_subnet);

  return applied;
}

/**
 * Write a wifi config record out to its config file, touching only the options
 * whose value actually changed so comments and ordering survive.
 */
bool writeWifiConfigFile(const wifi_config_table *_table)
{
  pdiutil::string name = CHARPTR_WRAP(WIFI_CONFIG_FEATURE_NAME);
  pdiutil::string path;
  buildConfigPath(name.c_str(), path);

  if (path.empty() || nullptr == _table)
  {
    return false;
  }

  pdiutil::vector<config_kv_t> desired;
  wifiConfigToKvs(_table, desired);

  pdiutil::string header = CHARPTR_WRAP(WIFI_CONFIG_HEADER);
  return writeConfigValues(path.c_str(), desired, header.c_str(), FILE_PERM_PRIVATE_FILE);
}

/**
 * Bring the wifi config file and the record store into agreement, the file
 * winning where it carries a key and being seeded from the record where not.
 */
bool syncWifiConfigFile()
{
  pdiutil::string name = CHARPTR_WRAP(WIFI_CONFIG_FEATURE_NAME);
  pdiutil::string path;
  buildConfigPath(name.c_str(), path);

  if (path.empty())
  {
    return false;
  }

  wifi_config_table table;
  __database_service.get_wifi_config_table(&table);

  pdiutil::vector<config_kv_t> present;
  if (__i_fs.isFileExist(path.c_str()))
  {
    loadConfigFile(path.c_str(), present);
  }

  wifiConfigFromKvs(present, &table);

  return __database_service.set_wifi_config_table(&table);
}

#endif

#if defined(ENABLE_MQTT_SERVICE) && defined(ENABLE_MQTT_CONFIG_FILE)

/**
 * Render the mqtt broker and last will records as the key/value pairs their
 * shared config file carries.
 */
void mqttConfigToKvs(const mqtt_general_config_table *_general, const mqtt_lwt_config_table *_lwt, pdiutil::vector<config_kv_t> &_out)
{
  _out.clear();

  if (nullptr == _general || nullptr == _lwt)
  {
    return;
  }

  pdiutil::string key_host = CHARPTR_WRAP(MQTT_CONFIG_KEY_HOST);
  pdiutil::string key_port = CHARPTR_WRAP(MQTT_CONFIG_KEY_PORT);
  pdiutil::string key_client_id = CHARPTR_WRAP(MQTT_CONFIG_KEY_CLIENT_ID);
  pdiutil::string key_username = CHARPTR_WRAP(MQTT_CONFIG_KEY_USERNAME);
  pdiutil::string key_password = CHARPTR_WRAP(MQTT_CONFIG_KEY_PASSWORD);
  pdiutil::string key_keepalive = CHARPTR_WRAP(MQTT_CONFIG_KEY_KEEPALIVE);
  pdiutil::string key_clean_session = CHARPTR_WRAP(MQTT_CONFIG_KEY_CLEAN_SESSION);
  pdiutil::string key_security = CHARPTR_WRAP(MQTT_CONFIG_KEY_SECURITY);
  pdiutil::string key_will_topic = CHARPTR_WRAP(MQTT_CONFIG_KEY_WILL_TOPIC);
  pdiutil::string key_will_message = CHARPTR_WRAP(MQTT_CONFIG_KEY_WILL_MESSAGE);
  pdiutil::string key_will_qos = CHARPTR_WRAP(MQTT_CONFIG_KEY_WILL_QOS);
  pdiutil::string key_will_retain = CHARPTR_WRAP(MQTT_CONFIG_KEY_WILL_RETAIN);

  appendConfigValue(_out, key_host, pdiutil::string(_general->host));
  appendConfigValue(_out, key_port, configNumberAsValue(_general->port));
  appendConfigValue(_out, key_client_id, pdiutil::string(_general->client_id));
  appendConfigValue(_out, key_username, pdiutil::string(_general->username));
  appendConfigValue(_out, key_password, pdiutil::string(_general->password));
  appendConfigValue(_out, key_keepalive, configNumberAsValue(_general->keepalive));
  appendConfigValue(_out, key_security, configBoolAsValue(0 != _general->security));
  appendConfigValue(_out, key_clean_session, configBoolAsValue(0 != _general->clean_session));

  appendConfigValue(_out, key_will_topic, pdiutil::string(_lwt->will_topic));
  appendConfigValue(_out, key_will_message, pdiutil::string(_lwt->will_message));
  appendConfigValue(_out, key_will_qos, configNumberAsValue(_lwt->will_qos));
  appendConfigValue(_out, key_will_retain, configBoolAsValue(0 != _lwt->will_retain));
}

/**
 * Take into the mqtt records whatever keys the config file supplied, so a key
 * the file does not carry keeps the value it arrived with.
 */
bool mqttConfigFromKvs(const pdiutil::vector<config_kv_t> &_kvs, mqtt_general_config_table *_general, mqtt_lwt_config_table *_lwt)
{
  if (nullptr == _general || nullptr == _lwt)
  {
    return false;
  }

  pdiutil::string key_host = CHARPTR_WRAP(MQTT_CONFIG_KEY_HOST);
  pdiutil::string key_port = CHARPTR_WRAP(MQTT_CONFIG_KEY_PORT);
  pdiutil::string key_client_id = CHARPTR_WRAP(MQTT_CONFIG_KEY_CLIENT_ID);
  pdiutil::string key_username = CHARPTR_WRAP(MQTT_CONFIG_KEY_USERNAME);
  pdiutil::string key_password = CHARPTR_WRAP(MQTT_CONFIG_KEY_PASSWORD);
  pdiutil::string key_keepalive = CHARPTR_WRAP(MQTT_CONFIG_KEY_KEEPALIVE);
  pdiutil::string key_clean_session = CHARPTR_WRAP(MQTT_CONFIG_KEY_CLEAN_SESSION);
  pdiutil::string key_security = CHARPTR_WRAP(MQTT_CONFIG_KEY_SECURITY);
  pdiutil::string key_will_topic = CHARPTR_WRAP(MQTT_CONFIG_KEY_WILL_TOPIC);
  pdiutil::string key_will_message = CHARPTR_WRAP(MQTT_CONFIG_KEY_WILL_MESSAGE);
  pdiutil::string key_will_qos = CHARPTR_WRAP(MQTT_CONFIG_KEY_WILL_QOS);
  pdiutil::string key_will_retain = CHARPTR_WRAP(MQTT_CONFIG_KEY_WILL_RETAIN);

  bool applied = false;

  applied |= takeConfigText(_kvs, key_host, _general->host, MQTT_HOST_BUF_SIZE);
  applied |= takeConfigNumber(_kvs, key_port, &_general->port);
  applied |= takeConfigBool(_kvs, key_security, &_general->security);
  applied |= takeConfigText(_kvs, key_client_id, _general->client_id, MQTT_CLIENT_ID_BUF_SIZE);
  applied |= takeConfigText(_kvs, key_username, _general->username, MQTT_USERNAME_BUF_SIZE);
  applied |= takeConfigText(_kvs, key_password, _general->password, MQTT_PASSWORD_BUF_SIZE);
  applied |= takeConfigNumber(_kvs, key_keepalive, &_general->keepalive);
  applied |= takeConfigBool(_kvs, key_clean_session, &_general->clean_session);

  applied |= takeConfigText(_kvs, key_will_topic, _lwt->will_topic, MQTT_TOPIC_BUF_SIZE);
  applied |= takeConfigText(_kvs, key_will_message, _lwt->will_message, MQTT_WILL_MSG_BUF_SIZE);
  applied |= takeConfigNumber(_kvs, key_will_qos, &_lwt->will_qos, MQTT_MAX_QOS_LEVEL);
  applied |= takeConfigBool(_kvs, key_will_retain, &_lwt->will_retain);

  return applied;
}

/**
 * Write both mqtt records out to their config file. The records are read back
 * from the store so a save of one of them cannot drop the other's options.
 */
bool writeMqttConfigFile()
{
  pdiutil::string name = CHARPTR_WRAP(MQTT_CONFIG_FEATURE_NAME);
  pdiutil::string path;
  buildConfigPath(name.c_str(), path);

  if (path.empty())
  {
    return false;
  }

  mqtt_general_config_table *general = pdiutil::safe_new<mqtt_general_config_table>();
  mqtt_lwt_config_table *lwt = pdiutil::safe_new<mqtt_lwt_config_table>();

  if (nullptr == general || nullptr == lwt)
  {
    pdiutil::safe_delete(general);
    pdiutil::safe_delete(lwt);
    return false;
  }

  __database_service.get_mqtt_general_config_table(general);
  __database_service.get_mqtt_lwt_config_table(lwt);

  pdiutil::vector<config_kv_t> desired;
  mqttConfigToKvs(general, lwt, desired);

  pdiutil::safe_delete(general);
  pdiutil::safe_delete(lwt);

  pdiutil::string header = CHARPTR_WRAP(MQTT_CONFIG_HEADER);
  return writeConfigValues(path.c_str(), desired, header.c_str(), FILE_PERM_PRIVATE_FILE);
}

/**
 * Bring the mqtt config file and the record store into agreement, the file
 * winning where it carries a key and being seeded from the records where not.
 */
bool syncMqttConfigFile()
{
  pdiutil::string name = CHARPTR_WRAP(MQTT_CONFIG_FEATURE_NAME);
  pdiutil::string path;
  buildConfigPath(name.c_str(), path);

  if (path.empty())
  {
    return false;
  }

  mqtt_general_config_table *general = pdiutil::safe_new<mqtt_general_config_table>();
  mqtt_lwt_config_table *lwt = pdiutil::safe_new<mqtt_lwt_config_table>();

  if (nullptr == general || nullptr == lwt)
  {
    pdiutil::safe_delete(general);
    pdiutil::safe_delete(lwt);
    return false;
  }

  __database_service.get_mqtt_general_config_table(general);
  __database_service.get_mqtt_lwt_config_table(lwt);

  pdiutil::vector<config_kv_t> present;
  if (__i_fs.isFileExist(path.c_str()))
  {
    loadConfigFile(path.c_str(), present);
  }

  mqttConfigFromKvs(present, general, lwt);

  bool status = __database_service.set_mqtt_general_config_table(general);
  status = __database_service.set_mqtt_lwt_config_table(lwt) && status;

  pdiutil::safe_delete(general);
  pdiutil::safe_delete(lwt);

  return status;
}

#endif

#if defined(ENABLE_OTA_SERVICE) && defined(ENABLE_OTA_CONFIG_FILE)

/**
 * Render an ota config record as the key/value pairs its config file carries.
 */
void otaConfigToKvs(const ota_config_table *_table, pdiutil::vector<config_kv_t> &_out)
{
  _out.clear();

  if (nullptr == _table)
  {
    return;
  }

  pdiutil::string key_host = CHARPTR_WRAP(OTA_CONFIG_KEY_HOST);
  pdiutil::string key_port = CHARPTR_WRAP(OTA_CONFIG_KEY_PORT);

  appendConfigValue(_out, key_host, pdiutil::string(_table->ota_host));
  appendConfigValue(_out, key_port, configNumberAsValue(_table->ota_port));
}

/**
 * Take into an ota config record whatever keys the config file supplied, so a
 * key the file does not carry keeps the value it arrived with.
 */
bool otaConfigFromKvs(const pdiutil::vector<config_kv_t> &_kvs, ota_config_table *_table)
{
  if (nullptr == _table)
  {
    return false;
  }

  pdiutil::string key_host = CHARPTR_WRAP(OTA_CONFIG_KEY_HOST);
  pdiutil::string key_port = CHARPTR_WRAP(OTA_CONFIG_KEY_PORT);

  bool applied = false;

  applied |= takeConfigText(_kvs, key_host, _table->ota_host, OTA_HOST_BUF_SIZE);
  applied |= takeConfigNumber(_kvs, key_port, &_table->ota_port);

  return applied;
}

/**
 * Write an ota config record out to its config file, touching only the options
 * whose value actually changed so comments and ordering survive.
 */
bool writeOtaConfigFile(const ota_config_table *_table)
{
  pdiutil::string name = CHARPTR_WRAP(OTA_CONFIG_FEATURE_NAME);
  pdiutil::string path;
  buildConfigPath(name.c_str(), path);

  if (path.empty() || nullptr == _table)
  {
    return false;
  }

  pdiutil::vector<config_kv_t> desired;
  otaConfigToKvs(_table, desired);

  pdiutil::string header = CHARPTR_WRAP(OTA_CONFIG_HEADER);
  return writeConfigValues(path.c_str(), desired, header.c_str(), FILE_PERM_PRIVATE_FILE);
}

/**
 * Bring the ota config file and the record store into agreement, the file
 * winning where it carries a key and being seeded from the record where not.
 */
bool syncOtaConfigFile()
{
  pdiutil::string name = CHARPTR_WRAP(OTA_CONFIG_FEATURE_NAME);
  pdiutil::string path;
  buildConfigPath(name.c_str(), path);

  if (path.empty())
  {
    return false;
  }

  ota_config_table table;
  __database_service.get_ota_config_table(&table);

  pdiutil::vector<config_kv_t> present;
  if (__i_fs.isFileExist(path.c_str()))
  {
    loadConfigFile(path.c_str(), present);
  }

  otaConfigFromKvs(present, &table);

  return __database_service.set_ota_config_table(&table);
}

#endif

#if defined(ENABLE_EMAIL_SERVICE) && defined(ENABLE_EMAIL_CONFIG_FILE)

/**
 * Render an email config record as the key/value pairs its config file carries.
 */
void emailConfigToKvs(const email_config_table *_table, pdiutil::vector<config_kv_t> &_out)
{
  _out.clear();

  if (nullptr == _table)
  {
    return;
  }

  pdiutil::string key_sending_domain = CHARPTR_WRAP(EMAIL_CONFIG_KEY_SENDING_DOMAIN);
  pdiutil::string key_host = CHARPTR_WRAP(EMAIL_CONFIG_KEY_HOST);
  pdiutil::string key_port = CHARPTR_WRAP(EMAIL_CONFIG_KEY_PORT);
  pdiutil::string key_username = CHARPTR_WRAP(EMAIL_CONFIG_KEY_USERNAME);
  pdiutil::string key_password = CHARPTR_WRAP(EMAIL_CONFIG_KEY_PASSWORD);
  pdiutil::string key_from = CHARPTR_WRAP(EMAIL_CONFIG_KEY_FROM);
  pdiutil::string key_from_name = CHARPTR_WRAP(EMAIL_CONFIG_KEY_FROM_NAME);
  pdiutil::string key_to = CHARPTR_WRAP(EMAIL_CONFIG_KEY_TO);
  pdiutil::string key_subject = CHARPTR_WRAP(EMAIL_CONFIG_KEY_SUBJECT);

  appendConfigValue(_out, key_sending_domain, pdiutil::string(_table->sending_domain));
  appendConfigValue(_out, key_host, pdiutil::string(_table->mail_host));
  appendConfigValue(_out, key_port, configNumberAsValue(_table->mail_port));
  appendConfigValue(_out, key_username, pdiutil::string(_table->mail_username));
  appendConfigValue(_out, key_password, pdiutil::string(_table->mail_password));

  appendConfigValue(_out, key_from, pdiutil::string(_table->mail_from));
  appendConfigValue(_out, key_from_name, pdiutil::string(_table->mail_from_name));
  appendConfigValue(_out, key_to, pdiutil::string(_table->mail_to));
  appendConfigValue(_out, key_subject, pdiutil::string(_table->mail_subject));
}

/**
 * Take into an email config record whatever keys the config file supplied, so a
 * key the file does not carry keeps the value it arrived with.
 */
bool emailConfigFromKvs(const pdiutil::vector<config_kv_t> &_kvs, email_config_table *_table)
{
  if (nullptr == _table)
  {
    return false;
  }

  pdiutil::string key_sending_domain = CHARPTR_WRAP(EMAIL_CONFIG_KEY_SENDING_DOMAIN);
  pdiutil::string key_host = CHARPTR_WRAP(EMAIL_CONFIG_KEY_HOST);
  pdiutil::string key_port = CHARPTR_WRAP(EMAIL_CONFIG_KEY_PORT);
  pdiutil::string key_username = CHARPTR_WRAP(EMAIL_CONFIG_KEY_USERNAME);
  pdiutil::string key_password = CHARPTR_WRAP(EMAIL_CONFIG_KEY_PASSWORD);
  pdiutil::string key_from = CHARPTR_WRAP(EMAIL_CONFIG_KEY_FROM);
  pdiutil::string key_from_name = CHARPTR_WRAP(EMAIL_CONFIG_KEY_FROM_NAME);
  pdiutil::string key_to = CHARPTR_WRAP(EMAIL_CONFIG_KEY_TO);
  pdiutil::string key_subject = CHARPTR_WRAP(EMAIL_CONFIG_KEY_SUBJECT);

  bool applied = false;

  applied |= takeConfigText(_kvs, key_sending_domain, _table->sending_domain, DEFAULT_SENDING_DOMAIN_MAX_SIZE);
  applied |= takeConfigText(_kvs, key_host, _table->mail_host, DEFAULT_MAIL_HOST_MAX_SIZE);
  applied |= takeConfigNumber(_kvs, key_port, &_table->mail_port);
  applied |= takeConfigText(_kvs, key_username, _table->mail_username, DEFAULT_MAIL_USERNAME_MAX_SIZE);
  applied |= takeConfigText(_kvs, key_password, _table->mail_password, DEFAULT_MAIL_PASSWORD_MAX_SIZE);

  applied |= takeConfigText(_kvs, key_from, _table->mail_from, DEFAULT_MAIL_FROM_MAX_SIZE);
  applied |= takeConfigText(_kvs, key_from_name, _table->mail_from_name, DEFAULT_MAIL_FROM_NAME_MAX_SIZE);
  applied |= takeConfigText(_kvs, key_to, _table->mail_to, DEFAULT_MAIL_TO_MAX_SIZE);
  applied |= takeConfigText(_kvs, key_subject, _table->mail_subject, DEFAULT_MAIL_SUBJECT_MAX_SIZE);

  return applied;
}

/**
 * Write an email config record out to its config file, touching only the
 * options whose value actually changed so comments and ordering survive.
 */
bool writeEmailConfigFile(const email_config_table *_table)
{
  pdiutil::string name = CHARPTR_WRAP(EMAIL_CONFIG_FEATURE_NAME);
  pdiutil::string path;
  buildConfigPath(name.c_str(), path);

  if (path.empty() || nullptr == _table)
  {
    return false;
  }

  pdiutil::vector<config_kv_t> desired;
  emailConfigToKvs(_table, desired);

  pdiutil::string header = CHARPTR_WRAP(EMAIL_CONFIG_HEADER);
  return writeConfigValues(path.c_str(), desired, header.c_str(), FILE_PERM_PRIVATE_FILE);
}

/**
 * Bring the email config file and the record store into agreement, the file
 * winning where it carries a key and being seeded from the record where not.
 */
bool syncEmailConfigFile()
{
  pdiutil::string name = CHARPTR_WRAP(EMAIL_CONFIG_FEATURE_NAME);
  pdiutil::string path;
  buildConfigPath(name.c_str(), path);

  if (path.empty())
  {
    return false;
  }

  email_config_table *table = pdiutil::safe_new<email_config_table>();

  if (nullptr == table)
  {
    return false;
  }

  __database_service.get_email_config_table(table);

  pdiutil::vector<config_kv_t> present;
  if (__i_fs.isFileExist(path.c_str()))
  {
    loadConfigFile(path.c_str(), present);
  }

  emailConfigFromKvs(present, table);

  bool status = __database_service.set_email_config_table(table);

  pdiutil::safe_delete(table);

  return status;
}

#endif

#endif
