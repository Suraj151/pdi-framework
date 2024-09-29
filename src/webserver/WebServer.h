/******************************** web server **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _HTTP_WEB_SERVER_H_
#define _HTTP_WEB_SERVER_H_

#include <webserver/controllers/HomeController.h>
#include <webserver/controllers/DashboardController.h>
#include <webserver/controllers/OtaController.h>
#include <webserver/controllers/WiFiConfigController.h>
#include <webserver/controllers/LoginController.h>
#ifdef ENABLE_GPIO_SERVICE
#include <webserver/controllers/GPIOController.h>
#endif
#ifdef ENABLE_MQTT_SERVICE
#include <webserver/controllers/MQTTController.h>
#endif
#ifdef ENABLE_EMAIL_SERVICE
#include <webserver/controllers/EmailConfigController.h>
#endif
#ifdef ENABLE_DEVICE_IOT
#include <webserver/controllers/DeviceIotController.h>
#endif


/**
 * EwWebServer class
 */
class EwWebServer {

  public:

    /**
     * EwWebServer constructor.
     */
    EwWebServer();
    /**
     * EwWebServer destructor.
     */
    ~EwWebServer();

    void start_server( iWiFiInterface* _wifi );
    void handle_clients( void );

  protected:
    /**
		 * @var	iWiFiServerInterface  m_server
		 */
    iWiFiServerInterface  *m_server;
    /**
		 * @var	iWiFiInterface*|&WiFi m_wifi
		 */
    iWiFiInterface        *m_wifi;

  private:
    /**
		 * @var	HomeController  m_home_controller
		 */
    HomeController        m_home_controller;

    /**
		 * @var	DashboardController  m_dashboard_controller
		 */
    DashboardController   m_dashboard_controller;

    /**
		 * @var	OtaController  m_ota_controller
		 */
    OtaController         m_ota_controller;

    /**
		 * @var	WiFiConfigController  m_wificonfig_controller
		 */
    WiFiConfigController  m_wificonfig_controller;

    /**
		 * @var	LoginController  m_login_controller
		 */
    LoginController       m_login_controller;

    #ifdef ENABLE_GPIO_SERVICE
    /**
		 * @var	GpioController  m_gpio_controller
		 */
    GpioController        m_gpio_controller;
    #endif

    #ifdef ENABLE_MQTT_SERVICE
    /**
		 * @var	MqttController  m_mqtt_controller
		 */
    MqttController        m_mqtt_controller;
    #endif
    #ifdef ENABLE_EMAIL_SERVICE
    /**
		 * @var	EmailConfigController  m_emailconfig_controller
		 */
    EmailConfigController m_emailconfig_controller;
    #endif
    #ifdef ENABLE_DEVICE_IOT
    /**
		 * @var	DeviceIotController  m_device_iot_controller
		 */
    DeviceIotController   m_device_iot_controller;
    #endif

};

extern EwWebServer __web_server;

#endif
