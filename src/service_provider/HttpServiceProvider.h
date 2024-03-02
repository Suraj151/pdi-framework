/*************************** Http Protocol service ****************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _HTTP_SERVICE_PROVIDER_H_
#define _HTTP_SERVICE_PROVIDER_H_

#include <service_provider/ServiceProvider.h>
#include <transports/http/HTTPClient.h>

/**
 * HttpServiceProvider class
 */
class HttpServiceProvider : public ServiceProvider {

  public:

    /**
     * HttpServiceProvider constructor.
     */
    HttpServiceProvider();

    /**
		 * HttpServiceProvider destructor
		 */
    ~HttpServiceProvider();

    /**
		 * @var	iHttpClientInterface*  m_http_client
		 */
    iHttpClientInterface  *m_http_client;
    /**
		 * @var	char array m_host
		 */
    char        m_host[HTTP_HOST_ADDR_MAX_SIZE];
    /**
		 * @var	int|80  m_port
		 */
    int         m_port;
    /**
		 * @var	int|HTTP_REQUEST_RETRY  m_retry
		 */
    int         m_retry;


    bool followHttpRequest( int _httpCode );
};

extern HttpServiceProvider __http_service;

#endif
