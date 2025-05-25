/************************* WiFi Server Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "WiFiServerInterface.h"

/**
 * WiFiHttpServerInterface constructor.
 */
WiFiHttpServerInterface::WiFiHttpServerInterface()
{
}

/**
 * WiFiHttpServerInterface destructor.
 */
WiFiHttpServerInterface::~WiFiHttpServerInterface()
{
}

/**
 * begin
 */
void WiFiHttpServerInterface::begin()
{
  this->m_server.begin();
}

/**
 * begin
 */
void WiFiHttpServerInterface::begin(uint16_t port)
{
  this->m_server.begin(port);
}

/**
 * handleClient
 */
void WiFiHttpServerInterface::handleClient()
{
  this->m_server.handleClient();
}

/**
 * close
 */
void WiFiHttpServerInterface::close()
{
  this->m_server.close();
}

/**
 * on
 */
void WiFiHttpServerInterface::on(const pdiutil::string &uri, CallBackVoidArgFn handler)
{
  this->m_server.on(uri.c_str(), handler);
}

/**
 * onNotFound
 * called when handler is not assigned
 */
void WiFiHttpServerInterface::onNotFound(CallBackVoidArgFn fn)
{
  this->m_server.onNotFound(fn);
}

/**
 * onFileUpload
 * handle file uploads
 */
void WiFiHttpServerInterface::onFileUpload(CallBackVoidArgFn fn)
{
  this->m_server.onFileUpload(fn);
}

/**
 * arg
 * get request argument value by name
 */
pdiutil::string WiFiHttpServerInterface::arg(const pdiutil::string &name)
{
  return this->m_server.arg(name.c_str()).c_str();
}

/**
 * hasArg
 * check if argument exists
 */
bool WiFiHttpServerInterface::hasArg(const pdiutil::string &name) const
{
  return this->m_server.hasArg(name.c_str());
}

/**
 * collectHeaders
 * set the request headers to collect
 */
void WiFiHttpServerInterface::collectHeaders(const char *headerKeys[], const size_t headerKeysCount)
{
  this->m_server.collectHeaders(headerKeys, headerKeysCount);
}

/**
 * header
 * get request header value by name
 */
pdiutil::string WiFiHttpServerInterface::header(const pdiutil::string &name)
{
  return this->m_server.header(name.c_str()).c_str();
}

/**
 * hasHeader
 * check if header exists
 */
bool WiFiHttpServerInterface::hasHeader(const pdiutil::string &name) const
{
  return this->m_server.hasHeader(name.c_str());
}

/**
 * send
 */
void WiFiHttpServerInterface::send(int code, const char *content_type, const char *content)
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
void WiFiHttpServerInterface::sendHeader(const pdiutil::string &name, const pdiutil::string &value, bool first)
{
  this->m_server.sendHeader(name.c_str(), value.c_str(), first);
}

WiFiHttpServerInterface __i_wifi_http_server;
