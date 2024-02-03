/*************************** Default Database *********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "DefaultDatabase.h"



/**
 * @var	GlobalTable	__global_table
 */
GlobalTable	__global_table;

/**
 * @var	LoginTable	__login_table
 */
LoginTable	__login_table;

/**
 * @var	WiFiTable	__wifi_table
 */
WiFiTable	__wifi_table;

/**
 * @var	OtaTable	__ota_table
 */
OtaTable	__ota_table;


#ifdef ENABLE_GPIO_SERVICE
/**
 * @var	GpioTable	__gpio_table
 */
GpioTable	__gpio_table;
#endif

#ifdef ENABLE_MQTT_SERVICE
/**
 * @var	MqttGeneralTable	__mqtt_general_table
 */
MqttGeneralTable	__mqtt_general_table;
/**
 * @var	MqttLwtTable	__mqtt_lwt_table
 */
MqttLwtTable	__mqtt_lwt_table;
/**
 * @var	MqttPubSubTable	__mqtt_pubsub_table
 */
MqttPubSubTable	__mqtt_pubsub_table;
#endif

#ifdef ENABLE_EMAIL_SERVICE
/**
 * @var	EmailTable	__email_table
 */
EmailTable	__email_table;
#endif

#ifdef ENABLE_DEVICE_IOT
/**
 * @var	DeviceIotTable	__device_iot_table
 */
DeviceIotTable	__device_iot_table;
#endif


/**
 * init all tables.
 */
void DefaultDatabase::init_default_database(){
  __database.init_database();
}

/**
 * clear all tables to their defaults value.
 */
void DefaultDatabase::clear_default_tables(){
  __database.clear_all();
}

/**
 * get/fetch global config table from database.
 *
 * @return status
 */
bool DefaultDatabase::get_global_config_table( global_config_table* _table ){
  return __global_table.get(_table);
}

/**
 * get/fetch login credential table from database.
 *
 * @return status
 */
bool DefaultDatabase::get_login_credential_table( login_credential_table* _table ){
  return __login_table.get(_table);
}

/**
 * get/fetch wifi config table from database.
 *
 * @return status
 */
bool DefaultDatabase::get_wifi_config_table( wifi_config_table* _table ){
  return __wifi_table.get(_table);
}

/**
 * get/fetch ota(over the air update) config table from database.
 *
 * @return status
 */
bool DefaultDatabase::get_ota_config_table( ota_config_table* _table ){
  return __ota_table.get(_table);
}

#ifdef ENABLE_GPIO_SERVICE
/**
 * get/fetch gpio config table from database.
 *
 * @return status
 */
bool DefaultDatabase::get_gpio_config_table( gpio_config_table* _table ){
  return __gpio_table.get(_table);
}
#endif

#ifdef ENABLE_MQTT_SERVICE
/**
 * get/fetch mqtt general config table from database.
 *
 * @return status
 */
bool DefaultDatabase::get_mqtt_general_config_table( mqtt_general_config_table* _table ){
  return __mqtt_general_table.get(_table);
}

/**
 * get/fetch mqtt lwt config table from database.
 *
 * @return status
 */
bool DefaultDatabase::get_mqtt_lwt_config_table( mqtt_lwt_config_table* _table ){
  return __mqtt_lwt_table.get(_table);
}

/**
 * get/fetch mqtt pubsub config table from database.
 *
 * @return status
 */
bool DefaultDatabase::get_mqtt_pubsub_config_table( mqtt_pubsub_config_table* _table ){
  return __mqtt_pubsub_table.get(_table);
}
#endif

#ifdef ENABLE_EMAIL_SERVICE
/**
 * get/fetch email config table from database.
 *
 * @return status
 */
bool DefaultDatabase::get_email_config_table( email_config_table* _table ){
  return __email_table.get(_table);
}
#endif

#ifdef ENABLE_DEVICE_IOT
/**
 * get/fetch device iot config table from database.
 *
 * @return status
 */
bool DefaultDatabase::get_device_iot_config_table( device_iot_config_table* _table ){
  return __device_iot_table.get(_table);
}
#endif

/**
 * set global config table in database.
 *
 * @param global_config_table* _table
 */
void DefaultDatabase::set_global_config_table( global_config_table* _table ){
  __global_table.set( _table );
}

/**
 * set login credential config table in database.
 *
 * @param login_credential_table* _table
 */
void DefaultDatabase::set_login_credential_table( login_credential_table* _table ){
  __login_table.set( _table );
}

/**
 * set wifi config table in database.
 *
 * @param wifi_config_table* _table
 */
void DefaultDatabase::set_wifi_config_table( wifi_config_table* _table ){
  __wifi_table.set( _table );
}

/**
 * set ota(over the air update) config table in database.
 *
 * @param ota_config_table* _table
 */
void DefaultDatabase::set_ota_config_table( ota_config_table* _table ){
  __ota_table.set( _table );
}

#ifdef ENABLE_GPIO_SERVICE
/**
 * set gpio config table in database.
 *
 * @param gpio_config_table* _table
 */
void DefaultDatabase::set_gpio_config_table( gpio_config_table* _table ){
  __gpio_table.set( _table );
}
#endif

#ifdef ENABLE_MQTT_SERVICE
/**
 * set mqtt general config table in database.
 *
 * @param mqtt_general_config_table* _table
 */
void DefaultDatabase::set_mqtt_general_config_table( mqtt_general_config_table* _table ){
  __mqtt_general_table.set( _table );
}

/**
 * set mqtt lwt config table in database.
 *
 * @param mqtt_lwt_config_table* _table
 */
void DefaultDatabase::set_mqtt_lwt_config_table( mqtt_lwt_config_table* _table ){
  __mqtt_lwt_table.set( _table );
}

/**
 * set mqtt pubsub config table in database.
 *
 * @param mqtt_pubsub_config_table* _table
 */
void DefaultDatabase::set_mqtt_pubsub_config_table( mqtt_pubsub_config_table* _table ){
  __mqtt_pubsub_table.set( _table );
}
#endif

#ifdef ENABLE_EMAIL_SERVICE
/**
 * set email config table in database.
 *
 * @param email_config_table* _table
 */
void DefaultDatabase::set_email_config_table( email_config_table* _table ){
  __email_table.set( _table );
}
#endif

#ifdef ENABLE_DEVICE_IOT
/**
 * set device iot config table in database.
 *
 * @param device_iot_config_table* _table
 */
void DefaultDatabase::set_device_iot_config_table( device_iot_config_table* _table ){
  __device_iot_table.set( _table );
}
#endif

DefaultDatabase __database_service;
