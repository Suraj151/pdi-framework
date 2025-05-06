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
 * @brief MIME type for HTML content.
 */
static const char TEXT_HTML_CONTENT[] PROG_RODT_ATTR = "text/html";

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
 * @enum HTTP_RETURN_CODE
 * @brief Defines HTTP response codes used by the web server.
 */
enum HTTP_RETURN_CODE
{
  HTTP_OK = 200,              ///< HTTP 200 OK: Request succeeded.
  HTTP_UNAUTHORIZED = 401,    ///< HTTP 401 Unauthorized: Authentication required.
  HTTP_NOT_FOUND = 404,       ///< HTTP 404 Not Found: Resource not found.
  HTTP_REDIRECT = 301         ///< HTTP 301 Redirect: Resource moved permanently.
};

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
  void collect_resource(iServerInterface *_server);

  // Protected members (commented out for now)
  // protected:

  /**
   * @var iServerInterface* m_server
   * @brief Pointer to the server interface implementation.
   */
  iServerInterface *m_server;

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
