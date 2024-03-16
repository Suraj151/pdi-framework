/************************* WiFi Server Interface *******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WIFI_SERVER_INTERFACE_H_
#define _WIFI_SERVER_INTERFACE_H_

#include "esp8266.h"
#include <interface/pdi/modules/wifi/iWiFiServerInterface.h>

// enum HTTP_Method { HTTP_ANY, HTTP_GET, HTTP_HEAD, HTTP_POST, HTTP_PUT, HTTP_PATCH, HTTP_DELETE, HTTP_OPTIONS };
// enum HTTP_Upload_Status { UPLOAD_FILE_START, UPLOAD_FILE_WRITE, UPLOAD_FILE_END,
//                         UPLOAD_FILE_ABORTED };

// typedef HTTPMethod HTTP_Method;
// typedef std::function<void(void)> THandlerFunction;

/**
 * WiFiServerInterface class
 */
class WiFiServerInterface : public iWiFiServerInterface
{

public:
  /**
   * WiFiServerInterface constructor.
   */
  WiFiServerInterface();
  /**
   * WiFiServerInterface destructor.
   */
  ~WiFiServerInterface();

  void begin() override;
  void begin(uint16_t port) override;
  void handleClient() override;
  void close() override;
  void stop() override;

  void on(const String &uri, CallBackVoidArgFn handler) override;
  void on(const String &uri, http_method_t method, CallBackVoidArgFn fn) override;
  void onNotFound(CallBackVoidArgFn fn) override;   // called when handler is not assigned
  void onFileUpload(CallBackVoidArgFn fn) override; // handle file uploads

  String arg(const String &name) override;                                              // get request argument value by name
  String arg(int i) override;                                                           // get request argument value by number
  String argName(int i) override;                                                       // get request argument name by number
  int args() const override;                                                            // get arguments count
  bool hasArg(const String &name) const override;                                       // check if argument exists
  void collectHeaders(const char *headerKeys[], const size_t headerKeysCount) override; // set the request headers to collect
  String header(const String &name) override;                                           // get request header value by name
  String header(int i) override;                                                        // get request header value by number
  String headerName(int i) override;                                                    // get request header name by number
  int headers() const override;                                                         // get header count
  bool hasHeader(const String &name) const override;                                    // check if header exists
  String hostHeader() override;                                                         // get request host header if available or empty String if not

  void send(int code, const char *content_type = nullptr, const String &content = String("")) override;
  void send(int code, char *content_type, const String &content) override;
  void send(int code, const String &content_type, const String &content) override;
  void send(int code, const char *content_type, const char *content) override;
  void send(int code, const char *content_type, const char *content, size_t content_length) override;
  void send(int code, const char *content_type, const uint8_t *content, size_t content_length) override;
  void sendHeader(const String &name, const String &value, bool first = false) override;

private:
  /**
   * @var	ESP8266WebServer  m_server
   */
  ESP8266WebServer m_server;
};

#endif
