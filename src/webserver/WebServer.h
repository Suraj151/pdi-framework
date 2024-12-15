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
#ifdef ENABLE_OTA_SERVICE
#include <webserver/controllers/OtaController.h>
#endif
#ifdef ENABLE_WIFI_SERVICE
#include <webserver/controllers/WiFiConfigController.h>
#endif
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
 * WebServer class
 */
class WebServer {

  public:

    /**
     * WebServer constructor.
     */
    WebServer();
    /**
     * WebServer destructor.
     */
    ~WebServer();

    bool start_server(iServerInterface *iServer);
    void handle_clients(void);

  protected:
    /**
		 * @var	iServerInterface  m_server
		 */
    iServerInterface      *m_server;

  private:
    /**
		 * @var	HomeController  m_home_controller
		 */
    HomeController        m_home_controller;

    /**
		 * @var	DashboardController  m_dashboard_controller
		 */
    DashboardController   m_dashboard_controller;

    #ifdef ENABLE_OTA_SERVICE
    /**
		 * @var	OtaController  m_ota_controller
		 */
    OtaController         m_ota_controller;
    #endif

    #ifdef ENABLE_WIFI_SERVICE
    /**
		 * @var	WiFiConfigController  m_wificonfig_controller
		 */
    WiFiConfigController  m_wificonfig_controller;
    #endif

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

extern WebServer __web_server;

#endif
