/***************************** database service *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "DatabaseServiceProvider.h"
#include <service_provider/device/FactoryResetServiceProvider.h>

/**
 * @var	GlobalTable	__global_table
 */
GlobalTable __global_table;

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

/**
 * Constructor
 */
DatabaseServiceProvider::DatabaseServiceProvider() : ServiceProvider(SERVICE_DATABASE, "DB")
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
  __i_db.beginConfigs(__i_db.getMaxDBSize());
  __database.init_database(__i_db.getMaxDBSize());

  #ifdef AUTO_FACTORY_RESET_ON_INVALID_CONFIGS
  __task_scheduler.setInterval( [&]() {
    if ( !__i_db.isValidConfigs() ){
      LogE("\n\nFound invalid configs.. starting factory reset..!\n\n");
      __database_service.clear_default_tables();
      __factory_reset.factory_reset();
    }
  }, MILLISECOND_DURATION_5000, __i_dvc_ctrl.millis_now() );
  #endif

  // clear config to default on factory reset event if enabled
  #ifdef CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET
  __utl_event.add_event_listener(EVENT_FACTORY_RESET, [&](void *e){
      LogI("\n\nClearing configs to default on factory reset event!\n\n");
    __database_service.clear_default_tables();
  });
  #endif

  return ServiceProvider::initService(arg);
}

/**
 * clear all tables to their defaults value.
 */
void DatabaseServiceProvider::clear_default_tables()
{
  __database.clear_all();
}

/**
 * get/fetch global config table from database.
 *
 * @return status
 */
bool DatabaseServiceProvider::get_global_config_table(global_config_table *_table)
{
  return __global_table.get(_table);
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
 * set global config table in database.
 *
 * @param global_config_table* _table
 */
void DatabaseServiceProvider::set_global_config_table(global_config_table *_table)
{
  __global_table.set(_table);
}

#if defined(ENABLE_HTTP_SERVER) || defined(ENABLE_AUTH_SERVICE)
/**
 * set login credential config table in database.
 *
 * @param login_credential_table* _table
 */
void DatabaseServiceProvider::set_login_credential_table(login_credential_table *_table)
{
  __login_table.set(_table);
}
#endif

#ifdef ENABLE_WIFI_SERVICE  
/**
 * set wifi config table in database.
 *
 * @param wifi_config_table* _table
 */
void DatabaseServiceProvider::set_wifi_config_table(wifi_config_table *_table)
{
  __wifi_table.set(_table);
}
#endif

#ifdef ENABLE_OTA_SERVICE  
/**
 * set ota(over the air update) config table in database.
 *
 * @param ota_config_table* _table
 */
void DatabaseServiceProvider::set_ota_config_table(ota_config_table *_table)
{
  __ota_table.set(_table);
}
#endif

#ifdef ENABLE_GPIO_SERVICE
/**
 * set gpio config table in database.
 *
 * @param gpio_config_table* _table
 */
void DatabaseServiceProvider::set_gpio_config_table(gpio_config_table *_table)
{
  __gpio_table.set(_table);
}
#endif

#ifdef ENABLE_MQTT_SERVICE
/**
 * set mqtt general config table in database.
 *
 * @param mqtt_general_config_table* _table
 */
void DatabaseServiceProvider::set_mqtt_general_config_table(mqtt_general_config_table *_table)
{
  __mqtt_general_table.set(_table);
}

/**
 * set mqtt lwt config table in database.
 *
 * @param mqtt_lwt_config_table* _table
 */
void DatabaseServiceProvider::set_mqtt_lwt_config_table(mqtt_lwt_config_table *_table)
{
  __mqtt_lwt_table.set(_table);
}

/**
 * set mqtt pubsub config table in database.
 *
 * @param mqtt_pubsub_config_table* _table
 */
void DatabaseServiceProvider::set_mqtt_pubsub_config_table(mqtt_pubsub_config_table *_table)
{
  __mqtt_pubsub_table.set(_table);
}
#endif

#ifdef ENABLE_EMAIL_SERVICE
/**
 * set email config table in database.
 *
 * @param email_config_table* _table
 */
void DatabaseServiceProvider::set_email_config_table(email_config_table *_table)
{
  __email_table.set(_table);
}
#endif

#ifdef ENABLE_DEVICE_IOT
/**
 * set device iot config table in database.
 *
 * @param device_iot_config_table* _table
 */
void DatabaseServiceProvider::set_device_iot_config_table(device_iot_config_table *_table)
{
  __device_iot_table.set(_table);
}
#endif

DatabaseServiceProvider __database_service;