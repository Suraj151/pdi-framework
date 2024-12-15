/****************************** Web Resource **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_HTTP_SERVER)

#include "WebResource.h"

/**
 * WebResourceProvider constructor
 */
WebResourceProvider::WebResourceProvider() : 
  m_server(nullptr),
  m_db_conn(nullptr)
{
}

/**
 * WebResourceProvider destructor
 */
WebResourceProvider::~WebResourceProvider()
{
  this->m_server = nullptr;
  this->m_db_conn = nullptr;
}

/**
 * collect resources for web services
 *
 * @param iServerInterface* _server
 */
void WebResourceProvider::collect_resource(iServerInterface *_server)
{
  this->m_server = _server;
  this->m_db_conn = &__database_service;
}

WebResourceProvider __web_resource;

#endif