/************************ i WiFi Server Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _I_WIFI_SERVER_INTERFACE_H_
#define _I_WIFI_SERVER_INTERFACE_H_

#include <config/Common.h>
#include <interface/interface_includes.h>

// forward declaration of derived class for this interface
class WiFiServerInterface;

/**
 * iWiFiServerInterface class
 */
class iWiFiServerInterface
{

public:
  /**
   * iWiFiServerInterface constructor.
   */
  iWiFiServerInterface() {}
  /**
   * iWiFiServerInterface destructor.
   */
  virtual ~iWiFiServerInterface() {}

  virtual void begin() = 0;
  virtual void begin(uint16_t port) = 0;
  virtual void handleClient() = 0;
  virtual void close() = 0;

  virtual void on(const std::string &uri, CallBackVoidArgFn handler) = 0;
  virtual void onNotFound(CallBackVoidArgFn fn) = 0;   // called when handler is not assigned
  virtual void onFileUpload(CallBackVoidArgFn fn) = 0; // handle file uploads

  virtual std::string arg(const std::string &name) = 0;                                    // get request argument value by name
  virtual bool hasArg(const std::string &name) const = 0;                                  // check if argument exists
  virtual void collectHeaders(const char *headerKeys[], const size_t headerKeysCount) = 0; // set the request headers to collect
  virtual std::string header(const std::string &name) = 0;                                 // get request header value by name
  virtual bool hasHeader(const std::string &name) const = 0;                               // check if header exists

  virtual void send(int code, const char *content_type = nullptr, const char *content = nullptr) = 0;
  virtual void sendHeader(const std::string &name, const std::string &value, bool first = false) = 0;
};

// derived class must define this
extern WiFiServerInterface __i_wifi_server;

#endif
