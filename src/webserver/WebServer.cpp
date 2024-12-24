/******************************** web server **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_HTTP_SERVER)

#include "WebServer.h"

/**
 * HttpServer constructor.
 */
HttpServer::HttpServer() : 
  m_server(nullptr)
{
}

/**
 * HttpServer destructor.
 */
HttpServer::~HttpServer()
{
  this->m_server = nullptr;
}

/**
 * start http server functionality. this requires server interface to be provided
 */
bool HttpServer::start_server(iServerInterface *iServer)
{
  bool bStatus = false;

  if( nullptr == iServer ){
    return bStatus;
  }

  this->m_server = iServer;
  __web_resource.collect_resource(this->m_server);

  for (int i = 0; i < Controller::m_controllers.size(); i++)
  {
    LogFmtI("booting : %s controller\n", Controller::m_controllers[i].controller->m_controller_name);
    Controller::m_controllers[i].controller->boot();
  }

  // here the list of headers to be recorded
  // const char * headerkeys[] = {"User-Agent", "Cookie"} ;
  const char *headerkeys[] = {"Cookie"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
  // ask server to track these headers
  this->m_server->collectHeaders(headerkeys, headerkeyssize);

  // start the server
  this->m_server->begin();
  LogI("HTTP server started !\n");
  bStatus = true;

  return bStatus;
}

/**
 * handle clients. Should be called in loop
 */
void HttpServer::handle_clients()
{
  this->m_server->handleClient();
}

HttpServer __web_server;

#endif
