/************** Over The Air firmware update service **************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _HTTP_OTA_SERVICE_PROVIDER_H_
#define _HTTP_OTA_SERVICE_PROVIDER_H_

#include <service_provider/ServiceProvider.h>
#include <service_provider/DatabaseServiceProvider.h>
#include <ESP8266httpUpdate.h>

/**
 * ota status enum
 */
enum http_ota_status{

  BEGIN_FAILED,
  CONFIG_ERROR,
  VERSION_CHECK_FAILED,
  VERSION_JSON_ERROR,
  ALREADY_LATEST_VERSION,
  UPDATE_FAILD,
  NO_UPDATES,
  UPDATE_OK,
  UNKNOWN
};

/**
 * OtaServiceProvider class
 */
class OtaServiceProvider : public ServiceProvider{

  public:

    /**
     * OtaServiceProvider constructor.
     */
    OtaServiceProvider();
    /**
     * OtaServiceProvider destructor.
     */
    ~OtaServiceProvider();

    void begin_ota( iWiFiClientInterface* _wifi_client, iHttpClientInterface* _http_client );
    void handleOta( void );
    http_ota_status handle( void );
    void printOtaConfigLogs( void );

    /**
     * @var	iWiFiClientInterface*|nullptr	m_wifi_client
     */
    iWiFiClientInterface  *m_wifi_client;

    /**
     * @var	iHttpClientInterface*|nullptr	m_http_client
     */
    iHttpClientInterface  *m_http_client;
};

extern OtaServiceProvider __ota_service;

#endif
