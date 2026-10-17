/*************************** databsse service ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2023
******************************************************************************/

#ifndef _DATABASE_SERVICE_PROVIDER_H_
#define _DATABASE_SERVICE_PROVIDER_H_

#include <service_provider/ServiceProvider.h>

#if defined(ENABLE_HTTP_SERVER) || defined(ENABLE_AUTH_SERVICE)
#include <database/tables/LoginTable.h>
#endif
#ifdef ENABLE_WIFI_SERVICE
#include <database/tables/WiFiTable.h>
#endif
#ifdef ENABLE_OTA_SERVICE
#include <database/tables/OtaTable.h>
#endif
#ifdef ENABLE_GPIO_SERVICE
#include <database/tables/GpioTable.h>
#endif
#ifdef ENABLE_MQTT_SERVICE
#include <database/tables/MqttGeneralTable.h>
#include <database/tables/MqttLwtTable.h>
#include <database/tables/MqttPubSubTable.h>
#endif
#ifdef ENABLE_EMAIL_SERVICE
#include <database/tables/EmailTable.h>
#endif
#ifdef ENABLE_DEVICE_IOT
#include <database/tables/DeviceIotTable.h>
#endif

/**
 * DatabaseServiceProvider class
 */
class DatabaseServiceProvider : public ServiceProvider
{

public:
  /**
   * DatabaseServiceProvider constructor.
   */
  DatabaseServiceProvider();
  /**
   * DatabaseServiceProvider destructor
   */
  ~DatabaseServiceProvider();

  bool initService(void *arg = nullptr) override;
  bool isEssentialService() const override { return true; }

  /**
   * reset every table to the defaults the device falls back to.
   */
  bool clear_default_tables();

  /**
   * take what the device is running now as the defaults it falls back to.
   */
  bool save_defaults();

  /**
   * put the defaults the device falls back to back in use.
   */
  bool restore_defaults();

  /**
   * write each feature's config file from its record, so a restored record is what the next service start reads.
   */
  void write_config_files();

  /**
   * whether the live database is the container on storage.
   */
  bool is_storage_tier() const { return m_storage_tier; }

  bool get_global_config_table(global_config_table *_table);
#if defined(ENABLE_HTTP_SERVER) || defined(ENABLE_AUTH_SERVICE)
  bool get_login_credential_table(login_credential_table *_table);
#endif
#ifdef ENABLE_WIFI_SERVICE  
  bool get_wifi_config_table(wifi_config_table *_table);
#endif
#ifdef ENABLE_OTA_SERVICE  
  bool get_ota_config_table(ota_config_table *_table);
#endif

#ifdef ENABLE_GPIO_SERVICE
  bool get_gpio_config_table(gpio_config_table *_table);
#endif

#ifdef ENABLE_MQTT_SERVICE
  bool get_mqtt_general_config_table(mqtt_general_config_table *_table);
  bool get_mqtt_lwt_config_table(mqtt_lwt_config_table *_table);
  bool get_mqtt_pubsub_config_table(mqtt_pubsub_config_table *_table);
#endif

#ifdef ENABLE_EMAIL_SERVICE
  bool get_email_config_table(email_config_table *_table);
#endif

#ifdef ENABLE_DEVICE_IOT
  bool get_device_iot_config_table(device_iot_config_table *_table);
#endif

  bool set_global_config_table(global_config_table *_table);
#if defined(ENABLE_HTTP_SERVER) || defined(ENABLE_AUTH_SERVICE)
  bool set_login_credential_table(login_credential_table *_table);
#endif
#ifdef ENABLE_WIFI_SERVICE  
  bool set_wifi_config_table(wifi_config_table *_table);
#endif
#ifdef ENABLE_OTA_SERVICE  
  bool set_ota_config_table(ota_config_table *_table);
#endif

#ifdef ENABLE_GPIO_SERVICE
  bool set_gpio_config_table(gpio_config_table *_table);
#endif

#ifdef ENABLE_MQTT_SERVICE
  bool set_mqtt_general_config_table(mqtt_general_config_table *_table);
  bool set_mqtt_lwt_config_table(mqtt_lwt_config_table *_table);
  bool set_mqtt_pubsub_config_table(mqtt_pubsub_config_table *_table);
#endif

#ifdef ENABLE_EMAIL_SERVICE
  bool set_email_config_table(email_config_table *_table);
#endif

#ifdef ENABLE_DEVICE_IOT
  bool set_device_iot_config_table(device_iot_config_table *_table);
#endif

private:
  void resolve_tiers();

  /**
   * @var bool m_storage_tier
   * @brief Whether the live database is the container on storage.
   */
  bool m_storage_tier;
};

extern DatabaseServiceProvider __database_service;

#endif
