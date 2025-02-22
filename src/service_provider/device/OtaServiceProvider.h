/************** Over The Air firmware update service **************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _HTTP_OTA_SERVICE_PROVIDER_H_
#define _HTTP_OTA_SERVICE_PROVIDER_H_

#include <service_provider/ServiceProvider.h>
#include <service_provider/database/DatabaseServiceProvider.h>
#include <transports/http/HTTPClient.h>

/**
 * ota status enum
 */
enum http_ota_status{
  GET_VERSION_FAILED,
  VERSION_NOT_FOUND,
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

    void begin_ota(iClientInterface *_client);
    void handleOta();
    http_ota_status handle();
    void printConfigToTerminal(iTerminalInterface *terminal) override;

    /**
     * @var	Http_Client*|nullptr	m_http_client
     */
    Http_Client  *m_http_client;
};

extern OtaServiceProvider __ota_service;

#endif
