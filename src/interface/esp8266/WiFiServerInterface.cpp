/************************* WiFi Server Interface *******************************
This file is part of the Ewings Esp Stack.

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
void WiFiServerInterface::on(const String &uri, CallBackVoidArgFn handler)
{
  this->m_server.on(uri, handler);
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
String WiFiServerInterface::arg(const String &name)
{
  return this->m_server.arg(name);
}

/**
 * hasArg
 * check if argument exists
 */
bool WiFiServerInterface::hasArg(const String &name) const
{
  return this->m_server.hasArg(name);
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
String WiFiServerInterface::header(const String &name)
{
  return this->m_server.header(name);
}

/**
 * hasHeader
 * check if header exists
 */
bool WiFiServerInterface::hasHeader(const String &name) const
{
  return this->m_server.hasHeader(name);
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
void WiFiServerInterface::sendHeader(const String &name, const String &value, bool first)
{
  this->m_server.sendHeader(name, value, first);
}

WiFiServerInterface __i_wifi_server;
