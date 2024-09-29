/************************* WiFi Server Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "WiFiServerInterface.h"

/**
 * WiFiServerInterface constructor.
 */
WiFiServerInterface::WiFiServerInterface()
{
}

/**
 * WiFiServerInterface destructor.
 */
WiFiServerInterface::~WiFiServerInterface()
{
}

/**
 * begin
 */
void WiFiServerInterface::begin()
{
  this->m_server.begin();
}

/**
 * begin
 */
void WiFiServerInterface::begin(uint16_t port)
{
  this->m_server.begin(port);
}

/**
 * handleClient
 */
void WiFiServerInterface::handleClient()
{
  this->m_server.handleClient();
}

/**
 * close
 */
void WiFiServerInterface::close()
{
  this->m_server.close();
}

/**
 * on
 */
void WiFiServerInterface::on(const std::string &uri, CallBackVoidArgFn handler)
{
  this->m_server.on(uri.c_str(), handler);
}

/**
 * onNotFound
 * called when handler is not assigned
 */
void WiFiServerInterface::onNotFound(CallBackVoidArgFn fn)
{
  this->m_server.onNotFound(fn);
}

/**
 * onFileUpload
 * handle file uploads
 */
void WiFiServerInterface::onFileUpload(CallBackVoidArgFn fn)
{
  this->m_server.onFileUpload(fn);
}

/**
 * arg
 * get request argument value by name
 */
std::string WiFiServerInterface::arg(const std::string &name)
{
  return this->m_server.arg(name.c_str()).c_str();
}

/**
 * hasArg
 * check if argument exists
 */
bool WiFiServerInterface::hasArg(const std::string &name) const
{
  return this->m_server.hasArg(name.c_str());
}

/**
 * collectHeaders
 * set the request headers to collect
 */
void WiFiServerInterface::collectHeaders(const char *headerKeys[], const size_t headerKeysCount)
{
  this->m_server.collectHeaders(headerKeys, headerKeysCount);
}

/**
 * header
 * get request header value by name
 */
std::string WiFiServerInterface::header(const std::string &name)
{
  return this->m_server.header(name.c_str()).c_str();
}

/**
 * hasHeader
 * check if header exists
 */
bool WiFiServerInterface::hasHeader(const std::string &name) const
{
  return this->m_server.hasHeader(name.c_str());
}

/**
 * send
 */
void WiFiServerInterface::send(int code, const char *content_type, const char *content)
{
  if( nullptr != content_type ){
    this->m_server.send(code, content_type, content);
  }else{
    this->m_server.send(code, "", "");
  }
}

/**
 * sendHeader
 */
void WiFiServerInterface::sendHeader(const std::string &name, const std::string &value, bool first)
{
  this->m_server.sendHeader(name.c_str(), value.c_str(), first);
}

WiFiServerInterface __i_wifi_server;
