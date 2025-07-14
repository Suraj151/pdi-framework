/****************************** Web Resource **********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `WebResourceProvider` class is responsible for managing and providing
resources required by the web server. It integrates with the server interface
and database service provider to handle dynamic content generation, HTTP
response codes, and resource collection.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_RESOURCE_PROVIDER_
#define _WEB_RESOURCE_PROVIDER_

#include <interface/pdi.h>
#include <utility/Utility.h>
#include <service_provider/database/DatabaseServiceProvider.h>
#include <webserver/routes/Routes.h>
#include <webserver/pages/Header.h>
#include <webserver/pages/Footer.h>
#include <webserver/pages/NotFound.h>
#include <webserver/helpers/DynamicPageBuildHelper.h>


/**
 * @define PAGE_HTML_MAX_SIZE
 * @brief Defines the maximum size of an HTML page.
 */
#define PAGE_HTML_MAX_SIZE 5000

/**
 * @define MIN_ACCEPTED_ARG_SIZE
 * @brief Defines the minimum accepted size for arguments in HTTP requests.
 */
#define MIN_ACCEPTED_ARG_SIZE 3

/**
 * @class WebResourceProvider
 * @brief Manages and provides resources for the web server.
 *
 * The `WebResourceProvider` class is responsible for collecting and managing
 * resources required by the web server. It integrates with the server interface
 * and database service provider to handle dynamic content generation and resource
 * management.
 */
class WebResourceProvider
{
public:
  /**
   * @brief Constructor for the `WebResourceProvider` class.
   *
   * Initializes the resource provider with default values.
   */
  WebResourceProvider();

  /**
   * @brief Destructor for the `WebResourceProvider` class.
   *
   * Cleans up resources used by the resource provider.
   */
  ~WebResourceProvider();

  /**
   * @brief Collects resources for the web server.
   *
   * This method integrates with the server interface to register and manage
   * resources required by the web server.
   *
   * @param _server Pointer to the server interface implementation.
   */
  void collect_resource(iHttpServerInterface *_server);

  // Protected members (commented out for now)
  // protected:

  /**
   * @var iHttpServerInterface* m_server
   * @brief Pointer to the server interface implementation.
   */
  iHttpServerInterface *m_server;

  /**
   * @var DatabaseServiceProvider* m_db_conn
   * @brief Pointer to the database service provider for managing database connections.
   */
  DatabaseServiceProvider *m_db_conn;
};

/**
 * @brief Global instance of the `WebResourceProvider` class.
 *
 * This instance is used to manage resources throughout the PDI stack.
 */
extern WebResourceProvider __web_resource;

#endif
