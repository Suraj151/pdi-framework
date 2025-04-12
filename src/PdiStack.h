/********************************* PDI stack *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _PDI_STACK_H_
#define _PDI_STACK_H_

#include <interface/pdi.h>

#ifdef ENABLE_HTTP_SERVER
#include <webserver/WebServer.h>
#endif

#include <service_provider/database/DatabaseServiceProvider.h>
#include <service_provider/device/FactoryResetServiceProvider.h>

#ifdef ENABLE_GPIO_SERVICE
#include <service_provider/device/GpioServiceProvider.h>
#endif

#ifdef ENABLE_WIFI_SERVICE
#include <service_provider/network/WiFiServiceProvider.h>
#endif

#ifdef ENABLE_OTA_SERVICE
#include <service_provider/device/OtaServiceProvider.h>
#endif

#ifdef ENABLE_MQTT_SERVICE
#include <service_provider/transport/MqttServiceProvider.h>
#endif

#ifdef ENABLE_EMAIL_SERVICE
#include <service_provider/email/EmailServiceProvider.h>
#endif

#ifdef ENABLE_DEVICE_IOT
#include <service_provider/iot/DeviceIotServiceProvider.h>
#endif

#ifdef ENABLE_SERIAL_SERVICE
#include <service_provider/transport/SerialServiceProvider.h>
#endif

#ifdef ENABLE_AUTH_SERVICE
#include <service_provider/auth/AuthServiceProvider.h>
#endif

#ifdef ENABLE_CMD_SERVICE
#include <service_provider/cmd/CommandLineServiceProvider.h>
#endif

/**
 * @class PDIStack
 * @brief The main class for managing the PDI stack.
 *
 * The PDIStack class serves as the central orchestrator for initializing and managing
 * various services and features of the PDI stack. It provides methods for initialization
 * and serving client requests, and it conditionally includes services based on preprocessor
 * directives.
 */
class PDIStack {

  public:

    /**
     * @brief Constructor for the PDIStack class.
     *
     * Initializes the PDIStack object and sets up utility interfaces and task scheduler limits.
     * If WiFi service is enabled, it initializes the client and server interfaces.
     */
    PDIStack();

    /**
     * @brief Destructor for the PDIStack class.
     *
     * Cleans up resources by setting client and server pointers to nullptr if WiFi service is enabled.
     */
    ~PDIStack();

    /**
     * @brief Initializes all required features, services, and actions.
     *
     * This method initializes various services such as database, WiFi, HTTP server, OTA, GPIO, MQTT,
     * email, device IoT, authentication, and command-line services based on the enabled preprocessor directives.
     */
    void initialize( void );

    /**
     * @brief Handles internal actions, client requests, and auto operations.
     *
     * This method serves the PDI stack by handling HTTP server clients, executing scheduled tasks,
     * and yielding control to the device controller.
     */
    void serve( void );

  protected:

    /**
     * @brief Prints logs at defined intervals.
     *
     * If the network service is enabled, this method logs the validity of the NTP time and the current NTP time.
     */
    static void handleLogPrints( void );

#ifdef ENABLE_WIFI_SERVICE
    /**
     * @var iClientInterface* m_client
     * @brief Pointer to the client interface for WiFi service.
     *
     * This pointer is used to manage client-side operations for the WiFi service.
     */
    iClientInterface  *m_client;

    /**
     * @var iServerInterface* m_server
     * @brief Pointer to the server interface for WiFi service.
     *
     * This pointer is used to manage server-side operations for the WiFi service.
     */
    iServerInterface  *m_server;
#endif
};

/**
 * @brief Global instance of the PDIStack class.
 *
 * This instance is used to manage and serve the PDI stack throughout the application.
 */
extern PDIStack PdiStack;

#endif
