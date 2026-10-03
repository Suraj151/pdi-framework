/***************************** database service *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "DatabaseServiceProvider.h"
#include <database/core/DatabaseLayout.h>
#include <interface/pdi/impl/modules/database/EepromDbStore.h>
#include <interface/pdi/impl/modules/database/FsDbStore.h>
#ifdef ENABLE_DB_SEALING
#include <database/core/DbKey.h>
#endif
#include <service_provider/device/FactoryResetServiceProvider.h>
#ifdef ENABLE_STORAGE_SERVICE
#include <helpers/FeatureConfigFiles.h>
#endif

#if defined(ENABLE_HTTP_SERVER) || defined(ENABLE_AUTH_SERVICE)
/**
 * @var	LoginTable	__login_table
 */
LoginTable __login_table;
#endif

#ifdef ENABLE_WIFI_SERVICE 
/**
 * @var	WiFiTable	__wifi_table
 */
WiFiTable __wifi_table;
#endif

#ifdef ENABLE_OTA_SERVICE 
/**
 * @var	OtaTable	__ota_table
 */
OtaTable __ota_table;
#endif

#ifdef ENABLE_GPIO_SERVICE
/**
 * @var	GpioTable	__gpio_table
 */
GpioTable __gpio_table;
#endif

#ifdef ENABLE_MQTT_SERVICE
/**
 * @var	MqttGeneralTable	__mqtt_general_table
 */
MqttGeneralTable __mqtt_general_table;
/**
 * @var	MqttLwtTable	__mqtt_lwt_table
 */
MqttLwtTable __mqtt_lwt_table;
/**
 * @var	MqttPubSubTable	__mqtt_pubsub_table
 */
MqttPubSubTable __mqtt_pubsub_table;
#endif

#ifdef ENABLE_EMAIL_SERVICE
/**
 * @var	EmailTable	__email_table
 */
EmailTable __email_table;
#endif

#ifdef ENABLE_DEVICE_IOT
/**
 * @var	DeviceIotTable	__device_iot_table
 */
DeviceIotTable __device_iot_table;
#endif

#ifdef AUTO_FACTORY_RESET_ON_INVALID_CONFIGS
static void factoryResetOnInvalidConfigs(){
    if ( !__db_layout.is_mounted() ){
      SysLogE("\n\nFound invalid configs.. starting factory reset..!\n\n");
      // __database_service.clear_default_tables();
      __factory_reset.factory_reset();
    }
}
#endif

/**
 * Constructor
 */
DatabaseServiceProvider::DatabaseServiceProvider() : ServiceProvider(SERVICE_DATABASE, RODT_ATTR("DB")), m_storage_tier(false)
{
}

/**
 * Destructor
 */
DatabaseServiceProvider::~DatabaseServiceProvider()
{
}

/**
 * init all DB tables.
 */
bool DatabaseServiceProvider::initService(void *arg)
{
  uint8_t _unregistered = __database.init_database();
  if (0 != _unregistered)
  {
    SysLogE("%u config tables did not register, their configs will not persist\n", (unsigned)_unregistered);
  }

  this->resolve_tiers();

  // clear config to default on factory reset event if enabled
  #ifdef CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET
  __utl_event.add_event_listener(EVENT_FACTORY_RESET, [&](void *e){
      LogI("\n\nClearing configs to default on factory reset event!\n\n");
    __database_service.clear_default_tables();
  });
  #endif

  #ifdef AUTO_FACTORY_RESET_ON_INVALID_CONFIGS
  factoryResetOnInvalidConfigs();
  this->serviceSetInterval( factoryResetOnInvalidConfigs, MILLISECOND_DURATION_5000, __i_dvc_ctrl.millis_now() );
  #endif

  iTerminalInterface *line = serviceBootLine();
  if (nullptr != line)
  {
    if (0 == _unregistered)
    {
      line->writeln_ro(RODT_ATTR("every config table registered"));
    }
    else
    {
      line->write((uint32_t)_unregistered);
      line->writeln_ro(RODT_ATTR(" config tables did not register"));
    }
  }

  return ServiceProvider::initService(arg);
}

/**
 * decide which medium the device runs its database on.
 *
 * The container on storage is preferred and the eeprom holds the defaults
 * behind it. A device with no storage, or one whose container cannot be opened,
 * runs on the eeprom itself.
 */
void DatabaseServiceProvider::resolve_tiers()
{
  m_storage_tier = false;

#ifdef ENABLE_DB_SEALING
  // the sealing key lives in the eeprom and is read into ram once, the tier the
  // device ends up running on does not change where it comes from
  __i_eeprom_dbstore.init();
  if (PDI_OK != __db_key.load())
  {
    SysLogE("DB sealing key unavailable, secret tables will not load\n");
  }
  __i_eeprom_dbstore.deinit();
#endif

#ifdef ENABLE_STORAGE_SERVICE
  if (PDI_OK == __i_fs_dbstore.init())
  {
    pdi_err_t _live = __db_layout.mount(&__i_fs_dbstore, __database.m_database_tables);

    if (PDI_OK == _live)
    {
      m_storage_tier = true;

      // a container that never existed, or one that was wiped, starts from
      // whatever defaults the eeprom is holding
      if (__db_layout.was_formatted())
      {
        SysLogW("DB container laid down fresh, taking the eeprom defaults\n");
        this->restore_defaults();
      }
    }
    else
    {
      SysLogE("DB container mount failed (%d), running on eeprom instead\n", (int)_live);
    }
  }
#endif

  if (!m_storage_tier)
  {
    __i_eeprom_dbstore.init();

    pdi_err_t _live = __db_layout.mount(&__i_eeprom_dbstore, __database.m_database_tables);

    if (PDI_OK != _live)
    {
      SysLogE("DB mount failed (%d)\n", (int)_live);
    }
  }

  if (__db_layout.is_mounted() && __db_layout.was_formatted())
  {
    __db_layout.set_firmware_version(FIRMWARE_VERSION);
    __db_layout.set_launch_year(LAUNCH_YEAR);
  }
}

/**
 * take what the device is running now as the defaults it falls back to.
 *
 * @return status
 */
bool DatabaseServiceProvider::save_defaults()
{
#ifdef ENABLE_STORAGE_SERVICE
  if (!m_storage_tier)
  {
    return false;
  }

  // the defaults layout lives only for this copy, holding a second one for the
  // life of the device would cost ram that is idle almost all of that time
  DbLayout _defaults;

  __i_eeprom_dbstore.init();

  bool _status = PDI_OK == _defaults.mount(&__i_eeprom_dbstore, __database.m_database_tables) &&
                 PDI_OK == _defaults.copy_from(__db_layout);

  __i_eeprom_dbstore.deinit();

  return _status;
#else
  return false;
#endif
}

/**
 * put the defaults the device falls back to back in use.
 *
 * @return status
 */
bool DatabaseServiceProvider::restore_defaults()
{
#ifdef ENABLE_STORAGE_SERVICE
  if (!m_storage_tier)
  {
    return false;
  }

  DbLayout _defaults;

  __i_eeprom_dbstore.init();

  bool _status = PDI_OK == _defaults.mount(&__i_eeprom_dbstore, __database.m_database_tables) &&
                 PDI_OK == __db_layout.copy_from(_defaults);

  __i_eeprom_dbstore.deinit();

  return _status;
#else
  return false;
#endif
}

/**
 * clear all tables to their defaults value.
 *
 * @return status
 */
bool DatabaseServiceProvider::clear_default_tables()
{
#ifdef ENABLE_STORAGE_SERVICE
  if (m_storage_tier)
  {
    return this->restore_defaults();
  }
#endif

  return __database.clear_all();
}

/**
 * get/fetch global config values from the database superblock.
 *
 * @return status
 */
bool DatabaseServiceProvider::get_global_config_table(global_config_table *_table)
{
  if (!__db_layout.is_mounted())
  {
    return false;
  }

  _table->clear();
  _table->firmware_version = __db_layout.firmware_version();
  _table->current_year = __db_layout.launch_year();

  return true;
}

#if defined(ENABLE_HTTP_SERVER) || defined(ENABLE_AUTH_SERVICE)
/**
 * get/fetch login credential table from database.
 *
 * @return status
 */
bool DatabaseServiceProvider::get_login_credential_table(login_credential_table *_table)
{
  return __login_table.get(_table);
}
#endif

#ifdef ENABLE_WIFI_SERVICE 
/**
 * get/fetch wifi config table from database.
 *
 * @return status
 */
bool DatabaseServiceProvider::get_wifi_config_table(wifi_config_table *_table)
{
  return __wifi_table.get(_table);
}
#endif

#ifdef ENABLE_OTA_SERVICE  
/**
 * get/fetch ota(over the air update) config table from database.
 *
 * @return status
 */
bool DatabaseServiceProvider::get_ota_config_table(ota_config_table *_table)
{
  return __ota_table.get(_table);
}
#endif

#ifdef ENABLE_GPIO_SERVICE
/**
 * get/fetch gpio config table from database.
 *
 * @return status
 */
bool DatabaseServiceProvider::get_gpio_config_table(gpio_config_table *_table)
{
  return __gpio_table.get(_table);
}
#endif

#ifdef ENABLE_MQTT_SERVICE
/**
 * get/fetch mqtt general config table from database.
 *
 * @return status
 */
bool DatabaseServiceProvider::get_mqtt_general_config_table(mqtt_general_config_table *_table)
{
  return __mqtt_general_table.get(_table);
}

/**
 * get/fetch mqtt lwt config table from database.
 *
 * @return status
 */
bool DatabaseServiceProvider::get_mqtt_lwt_config_table(mqtt_lwt_config_table *_table)
{
  return __mqtt_lwt_table.get(_table);
}

/**
 * get/fetch mqtt pubsub config table from database.
 *
 * @return status
 */
bool DatabaseServiceProvider::get_mqtt_pubsub_config_table(mqtt_pubsub_config_table *_table)
{
  return __mqtt_pubsub_table.get(_table);
}
#endif

#ifdef ENABLE_EMAIL_SERVICE
/**
 * get/fetch email config table from database.
 *
 * @return status
 */
bool DatabaseServiceProvider::get_email_config_table(email_config_table *_table)
{
  return __email_table.get(_table);
}
#endif

#ifdef ENABLE_DEVICE_IOT
/**
 * get/fetch device iot config table from database.
 *
 * @return status
 */
bool DatabaseServiceProvider::get_device_iot_config_table(device_iot_config_table *_table)
{
  return __device_iot_table.get(_table);
}
#endif

/**
 * set global config values in the database superblock.
 *
 * @param global_config_table* _table
 * @return status
 */
bool DatabaseServiceProvider::set_global_config_table(global_config_table *_table)
{
  if (!__db_layout.is_mounted())
  {
    return false;
  }

  __db_layout.set_firmware_version(_table->firmware_version);
  __db_layout.set_launch_year(_table->current_year);

  return true;
}

#if defined(ENABLE_HTTP_SERVER) || defined(ENABLE_AUTH_SERVICE)
/**
 * set login credential config table in database.
 *
 * @param login_credential_table* _table
 * @return status
 */
bool DatabaseServiceProvider::set_login_credential_table(login_credential_table *_table)
{
  return __login_table.set(_table);
}
#endif

#ifdef ENABLE_WIFI_SERVICE  
/**
 * set wifi config table in database.
 *
 * @param wifi_config_table* _table
 * @return status
 */
bool DatabaseServiceProvider::set_wifi_config_table(wifi_config_table *_table)
{
  bool status = __wifi_table.set(_table);

#ifdef ENABLE_WIFI_CONFIG_FILE
  writeWifiConfigFile(_table);
#endif

  return status;
}
#endif

#ifdef ENABLE_OTA_SERVICE  
/**
 * set ota(over the air update) config table in database.
 *
 * @param ota_config_table* _table
 * @return status
 */
bool DatabaseServiceProvider::set_ota_config_table(ota_config_table *_table)
{
  bool status = __ota_table.set(_table);

#ifdef ENABLE_OTA_CONFIG_FILE
  writeOtaConfigFile(_table);
#endif

  return status;
}
#endif

#ifdef ENABLE_GPIO_SERVICE
/**
 * set gpio config table in database.
 *
 * @param gpio_config_table* _table
 * @return status
 */
bool DatabaseServiceProvider::set_gpio_config_table(gpio_config_table *_table)
{
  return __gpio_table.set(_table);
}
#endif

#ifdef ENABLE_MQTT_SERVICE
/**
 * set mqtt general config table in database.
 *
 * @param mqtt_general_config_table* _table
 * @return status
 */
bool DatabaseServiceProvider::set_mqtt_general_config_table(mqtt_general_config_table *_table)
{
  bool status = __mqtt_general_table.set(_table);

#ifdef ENABLE_MQTT_CONFIG_FILE
  writeMqttConfigFile();
#endif

  return status;
}

/**
 * set mqtt lwt config table in database.
 *
 * @param mqtt_lwt_config_table* _table
 * @return status
 */
bool DatabaseServiceProvider::set_mqtt_lwt_config_table(mqtt_lwt_config_table *_table)
{
  bool status = __mqtt_lwt_table.set(_table);

#ifdef ENABLE_MQTT_CONFIG_FILE
  writeMqttConfigFile();
#endif

  return status;
}

/**
 * set mqtt pubsub config table in database.
 *
 * @param mqtt_pubsub_config_table* _table
 * @return status
 */
bool DatabaseServiceProvider::set_mqtt_pubsub_config_table(mqtt_pubsub_config_table *_table)
{
  return __mqtt_pubsub_table.set(_table);
}
#endif

#ifdef ENABLE_EMAIL_SERVICE
/**
 * set email config table in database.
 *
 * @param email_config_table* _table
 * @return status
 */
bool DatabaseServiceProvider::set_email_config_table(email_config_table *_table)
{
  bool status = __email_table.set(_table);

#ifdef ENABLE_EMAIL_CONFIG_FILE
  writeEmailConfigFile(_table);
#endif

  return status;
}
#endif

#ifdef ENABLE_DEVICE_IOT
/**
 * set device iot config table in database.
 *
 * @param device_iot_config_table* _table
 * @return status
 */
bool DatabaseServiceProvider::set_device_iot_config_table(device_iot_config_table *_table)
{
  device_iot_config_table *_stored = pdiutil::safe_new<device_iot_config_table>();

  if (nullptr != _stored)
  {
    __device_iot_table.get(_stored);
  }

  if (nullptr == _stored ||
      0 != strncmp(_stored->device_iot_host, _table->device_iot_host, DEVICE_IOT_HOST_BUF_SIZE) ||
      0 != strncmp(_stored->device_iot_duid, _table->device_iot_duid, DEVICE_IOT_DUID_MAX_LENGTH))
  {
    memset(_table->device_iot_key, 0, DEVICE_IOT_KEY_BUF_SIZE);
  }

  pdiutil::safe_delete(_stored);

  return __device_iot_table.set(_table);
}
#endif

DatabaseServiceProvider __database_service;