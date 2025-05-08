/******************************** web server **********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `HttpServer` class provides the core functionality for managing an HTTP
web server. It integrates various controllers to handle specific routes and
services, such as Home, Dashboard, OTA updates, WiFi configuration, GPIO,
MQTT, Email, and IoT device management. The server can be started, and it
handles incoming client requests.

Author          : Suraj I.
Created Date    : 1st June 2019
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
 * @class HttpServer
 * @brief Manages the HTTP web server and its associated controllers.
 *
 * The `HttpServer` class is responsible for starting the web server, handling
 * incoming client requests, and managing various controllers for specific
 * services. It integrates modular controllers that can be enabled or disabled
 * based on the configuration.
 */
class HttpServer : public ServiceProvider{

  public:
    /**
     * @brief Constructor for the `HttpServer` class.
     *
     * Initializes the HTTP server and its associated controllers.
     */
    HttpServer();

    /**
     * @brief Destructor for the `HttpServer` class.
     *
     * Cleans up resources used by the HTTP server.
     */
    ~HttpServer();

    /**
     * @brief Starts the HTTP server.
     *
     * This method initializes the server interface and prepares it to handle
     * incoming client requests.
     *
     * @param iServer Pointer to the server interface implementation.
     * @return True if the server started successfully, false otherwise.
     */
    bool initService(void *arg = nullptr) override;

    /**
     * @brief Handles incoming client requests.
     *
     * This method processes client requests and routes them to the appropriate
     * controller for handling.
     */
    void handle_clients(void);

  protected:
    /**
     * @var iServerInterface* m_server
     * @brief Pointer to the server interface implementation.
     */
    iServerInterface *m_server;

  private:
    /**
     * @var HomeController m_home_controller
     * @brief Controller for handling home-related routes.
     */
    HomeController m_home_controller;

    /**
     * @var DashboardController m_dashboard_controller
     * @brief Controller for handling dashboard-related routes.
     */
    DashboardController m_dashboard_controller;

    #ifdef ENABLE_OTA_SERVICE
    /**
     * @var OtaController m_ota_controller
     * @brief Controller for handling OTA update-related routes.
     */
    OtaController m_ota_controller;
    #endif

    #ifdef ENABLE_WIFI_SERVICE
    /**
     * @var WiFiConfigController m_wificonfig_controller
     * @brief Controller for handling WiFi configuration-related routes.
     */
    WiFiConfigController m_wificonfig_controller;
    #endif

    /**
     * @var LoginController m_login_controller
     * @brief Controller for handling login-related routes.
     */
    LoginController m_login_controller;

    #ifdef ENABLE_GPIO_SERVICE
    /**
     * @var GpioController m_gpio_controller
     * @brief Controller for handling GPIO-related routes.
     */
    GpioController m_gpio_controller;
    #endif

    #ifdef ENABLE_MQTT_SERVICE
    /**
     * @var MqttController m_mqtt_controller
     * @brief Controller for handling MQTT-related routes.
     */
    MqttController m_mqtt_controller;
    #endif

    #ifdef ENABLE_EMAIL_SERVICE
    /**
     * @var EmailConfigController m_emailconfig_controller
     * @brief Controller for handling email configuration-related routes.
     */
    EmailConfigController m_emailconfig_controller;
    #endif

    #ifdef ENABLE_DEVICE_IOT
    /**
     * @var DeviceIotController m_device_iot_controller
     * @brief Controller for handling IoT device-related routes.
     */
    DeviceIotController m_device_iot_controller;
    #endif
};

/**
 * @brief Global instance of the `HttpServer` class.
 *
 * This instance is used to manage the web server throughout the PDI stack.
 */
extern HttpServer __web_server;

#endif
