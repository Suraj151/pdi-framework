/******************************** web server **********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `WebServer.cpp` file implements the `HttpServer` class, which provides the
core functionality for managing an HTTP web server. It integrates various
controllers to handle specific routes and services, such as Home, Dashboard,
OTA updates, WiFi configuration, GPIO, MQTT, Email, and IoT device management.
The server can be started, and it handles incoming client requests.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_HTTP_SERVER)

#include "WebServer.h"

/**
 * @brief Constructor for the `HttpServer` class.
 *
 * Initializes the HTTP server with default values.
 */
HttpServer::HttpServer() : 
  m_server(nullptr),
  ServiceProvider(SERVICE_HTTP_SERVER, RODT_ATTR("HTTP Server"))
{
}

/**
 * @brief Destructor for the `HttpServer` class.
 *
 * Cleans up resources used by the HTTP server.
 */
HttpServer::~HttpServer()
{
  this->m_server = nullptr;
}

/**
 * @brief Starts the HTTP server.
 *
 * This method initializes the server interface and prepares it to handle
 * incoming client requests. It also boots all registered controllers and
 * configures the server to track specific HTTP headers.
 *
 * @param iServer Pointer to the server interface implementation.
 * @return True if the server started successfully, false otherwise.
 */
bool HttpServer::initService(void *arg)
{
  bool bStatus = false;
  iServerInterface  *iServer = reinterpret_cast<iServerInterface*>(arg);

  if (nullptr == iServer) {
    return bStatus;
  }

  // if(nullptr != m_terminal){
  //   m_terminal->writeln_ro(RODT_ATTR("Initializing HTTP Server"));
  // }

  this->m_server = iServer;

  // Collect resources for the server
  __web_resource.collect_resource(this->m_server);

  // Boot all registered controllers
  for (int i = 0; i < Controller::m_controllers.size(); i++) {
    // if(nullptr != m_terminal){
    //   m_terminal->write_ro(RODT_ATTR("  Booting "));
    //   m_terminal->write(Controller::m_controllers[i].controller->m_controller_name);
    //   m_terminal->writeln_ro(RODT_ATTR(" Controller"));
    // }
  
    // LogFmtI("Booting: %s controller\n", Controller::m_controllers[i].controller->m_controller_name);
    Controller::m_controllers[i].controller->boot();
  }

  // Define headers to be tracked by the server
  const char *headerkeys[] = {"Cookie"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
  this->m_server->collectHeaders(headerkeys, headerkeyssize);

  // Start the server
  this->m_server->begin();

  // LogI("HTTP server started!\n");

  bStatus = ServiceProvider::initService(arg);

  return bStatus;
}

/**
 * @brief Handles incoming client requests.
 *
 * This method processes client requests and routes them to the appropriate
 * controller for handling. It should be called in the main loop.
 */
void HttpServer::handle_clients()
{
  this->m_server->handleClient();
}

/**
 * @brief Global instance of the `HttpServer` class.
 *
 * This instance is used to manage the web server throughout the PDI stack.
 */
HttpServer __web_server;

#endif
