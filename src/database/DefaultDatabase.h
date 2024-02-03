/*************************** Default Database *********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _DEFAULT_DATABASE_H_
#define _DEFAULT_DATABASE_H_

#include <database/core/DatabaseTable.h>
#include "tables/GlobalTable.h"
#include "tables/LoginTable.h"
#include "tables/WiFiTable.h"
#include "tables/OtaTable.h"
#ifdef ENABLE_GPIO_SERVICE
#include "tables/GpioTable.h"
#endif
#ifdef ENABLE_MQTT_SERVICE
#include "tables/MqttGeneralTable.h"
#include "tables/MqttLwtTable.h"
#include "tables/MqttPubSubTable.h"
#endif
#ifdef ENABLE_EMAIL_SERVICE
#include "tables/EmailTable.h"
#endif
#ifdef ENABLE_DEVICE_IOT
#include "tables/DeviceIotTable.h"
#endif


/**
 * DefaultDatabase class
 */
class DefaultDatabase {

  public:

    /**
     * DefaultDatabase constructor.
     */
    DefaultDatabase(){
    }

    /**
     * DefaultDatabase destructor.
     */
    ~DefaultDatabase(){
    }

    void init_default_database();
    void clear_default_tables();

    bool get_global_config_table( global_config_table* _table );
    bool get_login_credential_table( login_credential_table* _table );
    bool get_wifi_config_table( wifi_config_table* _table );
    bool get_ota_config_table( ota_config_table* _table );

    #ifdef ENABLE_GPIO_SERVICE
    bool get_gpio_config_table( gpio_config_table* _table );
    #endif

    #ifdef ENABLE_MQTT_SERVICE
    bool get_mqtt_general_config_table( mqtt_general_config_table* _table );
    bool get_mqtt_lwt_config_table( mqtt_lwt_config_table* _table );
    bool get_mqtt_pubsub_config_table( mqtt_pubsub_config_table* _table );
    #endif

    #ifdef ENABLE_EMAIL_SERVICE
    bool get_email_config_table( email_config_table* _table );
    #endif

    #ifdef ENABLE_DEVICE_IOT
    bool get_device_iot_config_table( device_iot_config_table* _table );
    #endif

    void set_global_config_table( global_config_table* _table );
    void set_login_credential_table( login_credential_table* _table );
    void set_wifi_config_table( wifi_config_table* _table );
    void set_ota_config_table( ota_config_table* _table );

    #ifdef ENABLE_GPIO_SERVICE
    void set_gpio_config_table( gpio_config_table* _table );
    #endif

    #ifdef ENABLE_MQTT_SERVICE
    void set_mqtt_general_config_table( mqtt_general_config_table* _table );
    void set_mqtt_lwt_config_table( mqtt_lwt_config_table* _table );
    void set_mqtt_pubsub_config_table( mqtt_pubsub_config_table* _table );
    #endif

    #ifdef ENABLE_EMAIL_SERVICE
    void set_email_config_table( email_config_table* _table );
    #endif

    #ifdef ENABLE_DEVICE_IOT
    void set_device_iot_config_table( device_iot_config_table* _table );
    #endif

};

extern DefaultDatabase __database_service;

#endif
