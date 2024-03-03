/***************************** Ewings Esp Stack *******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EWINGS_ESP_STACK_H_
#define _EWINGS_ESP_STACK_H_

#include <config/Config.h>

#include <interface/pdi.h>

#ifdef ENABLE_EWING_HTTP_SERVER
#include <webserver/EwingsWebServer.h>
#endif

#include <service_provider/DatabaseServiceProvider.h>
#include <service_provider/NtpServiceProvider.h>
#include <service_provider/OtaServiceProvider.h>
#include <service_provider/WiFiServiceProvider.h>
#include <service_provider/EventServiceProvider.h>
#include <service_provider/FactoryResetServiceProvider.h>

#ifdef ENABLE_ESP_NOW
#include <service_provider/ESPNOWServiceProvider.h>
#endif

#ifdef ENABLE_GPIO_SERVICE
#include <service_provider/GpioServiceProvider.h>
#endif

#ifdef ENABLE_MQTT_SERVICE
#include <service_provider/MqttServiceProvider.h>
#endif

#ifdef ENABLE_EMAIL_SERVICE
#include <service_provider/EmailServiceProvider.h>
#endif

#ifdef ENABLE_EXCEPTION_NOTIFIER
#include <helpers/ExceptionsNotifier.h>
#endif

#ifdef ENABLE_DEVICE_IOT
#include <service_provider/DeviceIotServiceProvider.h>
#endif

#if defined( ENABLE_NAPT_FEATURE )
#include "lwip/lwip_napt.h"
#include "lwip/app/dhcpserver.h"
#elif defined( ENABLE_NAPT_FEATURE_LWIP_V2 )
#include <lwip/napt.h>
#include <lwip/dns.h>
#include <LwipDhcpServer-NonOS.h>
#endif

/**
 * EwingsEspStack class
 */
class EwingsEspStack {

  public:

    /**
     * EwingsEspStack constructor.
     */
    EwingsEspStack();
    /**
     * EwingsEspStack destructor.
     */
    ~EwingsEspStack();

    void initialize( void );
    void serve( void );

  protected:
    static void handleLogPrints( void );
    #if defined( ENABLE_NAPT_FEATURE ) || defined( ENABLE_NAPT_FEATURE_LWIP_V2 )
    void enable_napt_service( void );
    #endif

    /**
		 * @var	iWiFiInterface*   m_wifi
		 */
    iWiFiInterface        *m_wifi;
    /**
		 * @var	iWiFiClientInterface*  m_wifi_client
		 */
    iWiFiClientInterface  *m_wifi_client;
    /**
		 * @var	iHttpClientInterface*  m_http_client
		 */
    iHttpClientInterface  *m_http_client;
};

extern EwingsEspStack EwStack;

#endif
