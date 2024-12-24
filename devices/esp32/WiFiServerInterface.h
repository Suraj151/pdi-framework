/************************* WiFi Server Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WIFI_SERVER_INTERFACE_H_
#define _WIFI_SERVER_INTERFACE_H_

#include "esp32.h"
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

  void on(const pdiutil::string &uri, CallBackVoidArgFn handler) override;
  void onNotFound(CallBackVoidArgFn fn) override;   // called when handler is not assigned
  void onFileUpload(CallBackVoidArgFn fn) override; // handle file uploads

  pdiutil::string arg(const pdiutil::string &name) override;                                    // get request argument value by name
  bool hasArg(const pdiutil::string &name) const override;                                  // check if argument exists
  void collectHeaders(const char *headerKeys[], const size_t headerKeysCount) override; // set the request headers to collect
  pdiutil::string header(const pdiutil::string &name) override;                                 // get request header value by name
  bool hasHeader(const pdiutil::string &name) const override;                               // check if header exists

  void send(int code, const char *content_type = nullptr, const char *content = nullptr) override;
  void sendHeader(const pdiutil::string &name, const pdiutil::string &value, bool first = false) override;

private:
  /**
   * @var	WebServer  m_server
   */
  WebServer m_server;
};

#endif
