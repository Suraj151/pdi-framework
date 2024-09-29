/************************* WiFi Server Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _MOCKDEVICE_WIFI_SERVER_INTERFACE_H_
#define _MOCKDEVICE_WIFI_SERVER_INTERFACE_H_

#include "mockdevice.h"
#include <interface/pdi/modules/wifi/iWiFiServerInterface.h>


/**
 * WiFiServerInterface class
 */
class WiFiServerInterface : public iWiFiServerInterface
{

public:
  /**
   * WiFiServerInterface constructor.
   */
  WiFiServerInterface(){}
  /**
   * WiFiServerInterface destructor.
   */
  ~WiFiServerInterface(){}

  void begin() override{}
  void begin(uint16_t port) override{}
  void handleClient() override{}
  void close() override{}

  void on(const std::string &uri, CallBackVoidArgFn handler) override{}
  void onNotFound(CallBackVoidArgFn fn) override{}   // called when handler is not assigned
  void onFileUpload(CallBackVoidArgFn fn) override{} // handle file uploads

  std::string arg(const std::string &name) override{return "";}                                    // get request argument value by name
  bool hasArg(const std::string &name) const override{return 0;}                                  // check if argument exists
  void collectHeaders(const char *headerKeys[], const size_t headerKeysCount) override{} // set the request headers to collect
  std::string header(const std::string &name) override{return "";}                                 // get request header value by name
  bool hasHeader(const std::string &name) const override{return 0;}                               // check if header exists

  void send(int code, const char *content_type = nullptr, const char *content = nullptr) override{}
  void sendHeader(const std::string &name, const std::string &value, bool first = false) override{}

};

#endif
