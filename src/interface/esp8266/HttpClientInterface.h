/************************* HTTP Client Interface *******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _HTTP_CLIENT_INTERFACE_H_
#define _HTTP_CLIENT_INTERFACE_H_

#include "esp8266.h"
#include <interface/pdi/middlewares/iHttpClientInterface.h>

/**
 * HttpClientInterface class
 */
class HttpClientInterface : public iHttpClientInterface
{

public:
  /**
   * HttpClientInterface constructor.
   */
  HttpClientInterface();
  /**
   * HttpClientInterface destructor.
   */
  ~HttpClientInterface();

  bool begin(iWiFiClientInterface &client, const String &url) override;
  bool begin(iWiFiClientInterface &client, const String &host, uint16_t port, const String &uri = "/", bool https = false) override;
  void end(void) override;
  bool connected(void) override;

  // void setReuse(bool reuse) override;
  void setUserAgent(const String &userAgent) override;
  void setAuthorization(const char *user, const char *password) override;
  // void setAuthorization(const char *auth) override;
  void setTimeout(uint16_t timeout) override;
  void setFollowRedirects(follow_redirects_t follow) override;
  void setRedirectLimit(uint16_t limit) override;
  // bool setURL(const String &url) override;

  int GET() override;
  int POST(const uint8_t *payload, size_t size) override;
  int POST(const String &payload) override;
  int PUT(const uint8_t *payload, size_t size) override;
  int PUT(const String &payload) override;
  int PATCH(const uint8_t *payload, size_t size) override;
  int PATCH(const String &payload) override;

  void addHeader(const String &name, const String &value, bool first = false, bool replace = true) override;
  void collectHeaders(const char *headerKeys[], const size_t headerKeysCount) override;
  String header(const char *name) override;
  String header(size_t i) override;
  String headerName(size_t i) override;
  int headers() override;
  bool hasHeader(const char *name) override;

  int getSize(void) override;
  const String &getLocation(void) override;
  WiFiClient &getStream(void) override;
  const String &getString(void) override;

private:
  /**
   * @var	HTTPClient
   */
  HTTPClient m_http_client;
};

#endif
