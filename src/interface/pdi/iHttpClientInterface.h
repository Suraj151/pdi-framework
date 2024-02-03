/**************************** HTTP Client Interface ****************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _I_HTTP_CLIENT_INTERFACE_H_
#define _I_HTTP_CLIENT_INTERFACE_H_

#include "iWiFiClientInterface.h"

// forward declaration of derived class for this interface
class HttpClientInterface;

/**
 * iHttpClientInterface class
 */
class iHttpClientInterface
{

public:
  /**
   * iHttpClientInterface constructor.
   */
  iHttpClientInterface() {}
  /**
   * iHttpClientInterface destructor.
   */
  virtual ~iHttpClientInterface() {}

  virtual bool begin(iWiFiClientInterface &client, const String &url) = 0;
  virtual bool begin(iWiFiClientInterface &client, const String &host, uint16_t port, const String &uri = "/", bool https = false) = 0;
  virtual void end(void) = 0;
  virtual bool connected(void) = 0;

  virtual void setReuse(bool reuse) = 0;
  virtual void setUserAgent(const String &userAgent) = 0;
  virtual void setAuthorization(const char *user, const char *password) = 0;
  virtual void setAuthorization(const char *auth) = 0;
  virtual void setTimeout(uint16_t timeout) = 0;
  virtual void setFollowRedirects(follow_redirects_t follow) = 0;
  virtual void setRedirectLimit(uint16_t limit) = 0;
  virtual bool setURL(const String &url) = 0;

  virtual int GET() = 0;
  virtual int POST(const uint8_t *payload, size_t size) = 0;
  virtual int POST(const String &payload) = 0;
  virtual int PUT(const uint8_t *payload, size_t size) = 0;
  virtual int PUT(const String &payload) = 0;
  virtual int PATCH(const uint8_t *payload, size_t size) = 0;
  virtual int PATCH(const String &payload) = 0;

  virtual void addHeader(const String &name, const String &value, bool first = false, bool replace = true) = 0;
  virtual void collectHeaders(const char *headerKeys[], const size_t headerKeysCount) = 0;
  virtual String header(const char *name) = 0;
  virtual String header(size_t i) = 0;
  virtual String headerName(size_t i) = 0;
  virtual int headers() = 0;
  virtual bool hasHeader(const char *name) = 0;

  virtual int getSize(void) = 0;
  virtual const String &getLocation(void) = 0;
  virtual WiFiClient &getStream(void) = 0;
  virtual const String &getString(void) = 0;
};

// derived class must define this
extern HttpClientInterface __i_http_client;

#endif
