/****************************** Web Resource **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_RESOURCE_PROVIDER_
#define _WEB_RESOURCE_PROVIDER_

#include <interface/pdi.h>
#include <utility/Utility.h>
#include <service_provider/DatabaseServiceProvider.h>
#include <webserver/routes/Routes.h>
#include <webserver/pages/Header.h>
#include <webserver/pages/Footer.h>
#include <webserver/pages/NotFound.h>
#include <webserver/helpers/DynamicPageBuildHelper.h>

static const char TEXT_HTML_CONTENT[] PROGMEM = "text/html";

/**
 * @define html page max size
 */
#define PAGE_HTML_MAX_SIZE 5000
#define MIN_ACCEPTED_ARG_SIZE 3

/**
 * @enum http return codes
 */
enum HTTP_RETURN_CODE
{
  HTTP_OK = 200,
  HTTP_UNAUTHORIZED = 401,
  HTTP_NOT_FOUND = 404,
  HTTP_REDIRECT = 301
};

/**
 * WebResourceProvider class
 */
class WebResourceProvider
{

public:
  /**
   * WebResourceProvider constructor
   */
  WebResourceProvider();
  /**
   * WebResourceProvider destructor
   */
  ~WebResourceProvider();

  void collect_resource(iWiFiInterface *_wifi, iWiFiServerInterface *_server);

  // protected:

  /**
   * @var	iWiFiInterface*  m_wifi
   */
  iWiFiInterface *m_wifi;
  /**
   * @var	iWiFiServerInterface*	m_server
   */
  iWiFiServerInterface *m_server;
  /**
   * @var	DatabaseServiceProvider*  m_db_conn
   */
  DatabaseServiceProvider *m_db_conn;
};

extern WebResourceProvider __web_resource;

#endif
