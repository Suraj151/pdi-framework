/************************* HTTP Client Interface *******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "HttpClientInterface.h"

/**
 * HttpClientInterface constructor.
 */
HttpClientInterface::HttpClientInterface()
{
}

/**
 * HttpClientInterface destructor.
 */
HttpClientInterface::~HttpClientInterface()
{
}

/**
 * begin
 */
bool HttpClientInterface::begin(iWiFiClientInterface &client, const String &url)
{
  return this->m_http_client.begin(*(client.getWiFiClient()), url);
}

/**
 * begin
 */
bool HttpClientInterface::begin(iWiFiClientInterface &client, const String &host, uint16_t port, const String &uri, bool https)
{
  return this->m_http_client.begin(*(client.getWiFiClient()), host, port, uri, https);
}

/**
 * end
 */
void HttpClientInterface::end(void)
{
  this->m_http_client.end();
}

/**
 * connected
 */
bool HttpClientInterface::connected(void)
{
  return this->m_http_client.connected();
}

/**
 * setReuse
 */
// void HttpClientInterface::setReuse(bool reuse)
// {
//   this->m_http_client.setReuse(reuse);
// }

/**
 * setUserAgent
 */
void HttpClientInterface::setUserAgent(const String &userAgent)
{
  this->m_http_client.setUserAgent(userAgent);
}

/**
 * setAuthorization
 */
void HttpClientInterface::setAuthorization(const char *user, const char *password)
{
  this->m_http_client.setAuthorization(user, password);
}

/**
 * setAuthorization
 */
// void HttpClientInterface::setAuthorization(const char *auth)
// {
//   this->m_http_client.setAuthorization(auth);
// }

/**
 * setTimeout
 */
void HttpClientInterface::setTimeout(uint16_t timeout)
{
  this->m_http_client.setTimeout(timeout);
}

/**
 * setFollowRedirects
 */
void HttpClientInterface::setFollowRedirects(follow_redirects_t follow)
{
  followRedirects_t _follow = static_cast<followRedirects_t>(follow);
  this->m_http_client.setFollowRedirects(_follow);
}

/**
 * setRedirectLimit
 */
void HttpClientInterface::setRedirectLimit(uint16_t limit)
{
  this->m_http_client.setRedirectLimit(limit);
}

/**
 * setURL
 */
// bool HttpClientInterface::setURL(const String &url)
// {
//   return this->m_http_client.setURL(url);
// }

/**
 * GET
 */
int HttpClientInterface::GET()
{
  return this->m_http_client.GET();
}

/**
 * POST
 */
int HttpClientInterface::POST(const uint8_t *payload, size_t size)
{
  return this->m_http_client.POST(payload, size);
}

/**
 * POST
 */
int HttpClientInterface::POST(const String &payload)
{
  return this->m_http_client.POST(payload);
}

/**
 * PUT
 */
int HttpClientInterface::PUT(const uint8_t *payload, size_t size)
{
  return this->m_http_client.PUT(payload, size);
}

/**
 * PUT
 */
int HttpClientInterface::PUT(const String &payload)
{
  return this->m_http_client.PUT(payload);
}

/**
 * PATCH
 */
int HttpClientInterface::PATCH(const uint8_t *payload, size_t size)
{
  return this->m_http_client.PATCH(payload, size);
}

/**
 * PATCH
 */
int HttpClientInterface::PATCH(const String &payload)
{
  return this->m_http_client.PATCH(payload);
}

/**
 * addHeader
 */
void HttpClientInterface::addHeader(const String &name, const String &value, bool first, bool replace)
{
  this->m_http_client.addHeader(name, value, first, replace);
}

/**
 * collectHeaders
 */
void HttpClientInterface::collectHeaders(const char *headerKeys[], const size_t headerKeysCount)
{
  this->m_http_client.collectHeaders(headerKeys, headerKeysCount);
}

/**
 * header
 */
String HttpClientInterface::header(const char *name)
{
  return this->m_http_client.header(name);
}

/**
 * header
 */
String HttpClientInterface::header(size_t i)
{
  return this->m_http_client.header(i);
}

/**
 * headerName
 */
String HttpClientInterface::headerName(size_t i)
{
  return this->m_http_client.headerName(i);
}

/**
 * headers
 */
int HttpClientInterface::headers()
{
  return this->m_http_client.headers();
}

/**
 * hasHeader
 */
bool HttpClientInterface::hasHeader(const char *name)
{
  return this->m_http_client.hasHeader(name);
}

/**
 * getSize
 */
int HttpClientInterface::getSize(void)
{
  return this->m_http_client.getSize();
}

/**
 * getLocation
 */
const String &HttpClientInterface::getLocation(void)
{
  return this->m_http_client.getLocation();
}

/**
 * getStream
 */
WiFiClient &HttpClientInterface::getStream(void)
{
  return this->m_http_client.getStream();
}

/**
 * getString
 */
const String &HttpClientInterface::getString(void)
{
  return this->m_http_client.getString();
}

HttpClientInterface __i_http_client;
