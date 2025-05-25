/****************************** Web Resource **********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `WebResource.cpp` file implements the `WebResourceProvider` class, which is
responsible for managing and providing resources required by the web server. It
integrates with the server interface and database service provider to handle
dynamic content generation and resource collection.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_HTTP_SERVER)

#include "WebResource.h"

/**
 * @brief Constructor for the `WebResourceProvider` class.
 *
 * Initializes the resource provider with default values, setting the server
 * and database connection pointers to `nullptr`.
 */
WebResourceProvider::WebResourceProvider() : 
  m_server(nullptr),
  m_db_conn(nullptr)
{
}

/**
 * @brief Destructor for the `WebResourceProvider` class.
 *
 * Cleans up resources used by the resource provider by resetting the server
 * and database connection pointers to `nullptr`.
 */
WebResourceProvider::~WebResourceProvider()
{
  this->m_server = nullptr;
  this->m_db_conn = nullptr;
}

/**
 * @brief Collects resources for the web server.
 *
 * This method integrates with the server interface to register and manage
 * resources required by the web server. It also initializes the database
 * connection using the global `__database_service` instance.
 *
 * @param _server Pointer to the server interface implementation.
 */
void WebResourceProvider::collect_resource(iHttpServerInterface *_server)
{
  this->m_server = _server;
  this->m_db_conn = &__database_service;
}

/**
 * @brief Global instance of the `WebResourceProvider` class.
 *
 * This instance is used to manage resources throughout the PDI stack.
 */
WebResourceProvider __web_resource;

#endif