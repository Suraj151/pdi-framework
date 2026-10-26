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
  ServiceProvider(SERVICE_HTTP_SERVER, RODT_ATTR("HTTPServer"))
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
  iHttpServerInterface  *iServer = reinterpret_cast<iHttpServerInterface*>(arg);

  if (nullptr == iServer) {
    iServer = &__i_http_server;
  }

  // if(nullptr != m_terminal){
  //   m_terminal->writeln_ro(RODT_ATTR("Initializing HTTP Server"));
  // }

  this->m_server = iServer;

  // Collect resources for the server
  __web_resource.collect_resource(this->m_server);

  // Boot all registered controllers
  for (int i = 0; i < Controller::ControllerRegistry().size(); i++) {
    // if(nullptr != m_terminal){
    //   m_terminal->write_ro(RODT_ATTR("  Booting "));
    //   m_terminal->write(Controller::ControllerRegistry()[i].controller->m_controller_name);
    //   m_terminal->writeln_ro(RODT_ATTR(" Controller"));
    // }
  
    // LogI("Booting: %s controller\n", Controller::ControllerRegistry()[i].controller->m_controller_name);
    Controller::ControllerRegistry()[i].controller->boot();
  }

  // Define headers to be tracked by the server
  const char *headerkeys[] = {"Cookie", "Host"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
  this->m_server->collectHeaders(headerkeys, headerkeyssize);
  this->m_server->setUploadGate([](){ return __web_route_handler.request_has_session(); });

  // Start the server
  #if defined(ENABLE_HTTPS_SERVER) && defined(ENABLE_TLS_SERVICE)
  this->m_server->setServerCertificatePath(TLS_DEFAULT_SERVER_CERT_PATH);
  this->m_server->setServerPrivateKeyPath(TLS_DEFAULT_SERVER_KEY_PATH);
  #ifdef ENABLE_HTTPS_SERVER_MTLS
  this->m_server->setClientCertificateAuthorityPath(TLS_DEFAULT_CLIENT_CA_PATH);
  #endif
  this->m_server->begin(HTTPS_DEFAULT_PORT, true);
  #else
  this->m_server->begin(HTTP_DEFAULT_PORT);
  #endif

  iTerminalInterface *line = serviceBootLine();
  if (nullptr != line) {
    line->write_ro(RODT_ATTR("listening on port "));
    #if defined(ENABLE_HTTPS_SERVER) && defined(ENABLE_TLS_SERVICE)
    line->write((uint32_t)HTTPS_DEFAULT_PORT);
    line->write_ro(RODT_ATTR(" over tls, "));
    #else
    line->write((uint32_t)HTTP_DEFAULT_PORT);
    line->write_ro(RODT_ATTR(", "));
    #endif
    line->write((uint32_t)Controller::ControllerRegistry().size());
    line->writeln_ro(RODT_ATTR(" controllers"));
  }

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
  // a service that never started has no listener to serve, and the loop calls
  // this on every pass whether it started or not
  if (nullptr == this->m_server) {
    return;
  }

  this->m_server->handleClient();
}

/**
 * Closes the listener, so a stopped http service refuses a request rather than
 * holding the browser open on a port nothing is serving.
 */
bool HttpServer::stopService()
{
  if (nullptr != this->m_server) {
    this->m_server->close();
  }
  return ServiceProvider::stopService();
}

/**
 * @brief Global instance of the `HttpServer` class.
 *
 * This instance is used to manage the web server throughout the PDI stack.
 */
HttpServer __web_server;

#endif
