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
 * WebServer constructor.
 */
WebServer::WebServer() : m_wifi(nullptr),
                             m_server(&__i_wifi_server)
{
}

/**
 * WebServer destructor.
 */
WebServer::~WebServer()
{
  this->m_wifi = nullptr;
}

/**
 * start http server functionality. this requires wifi should work as access point
 */
void WebServer::start_server(iWiFiInterface *_wifi)
{
  this->m_wifi = _wifi;
  __web_resource.collect_resource(this->m_wifi, this->m_server);

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
}

/**
 * handle server functionality.
 */
void WebServer::handle_clients()
{
  this->m_server->handleClient();
}

WebServer __web_server;

#endif
