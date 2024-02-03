/************************ i WiFi Client Interface ******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _I_WIFI_CLIENT_INTERFACE_H_
#define _I_WIFI_CLIENT_INTERFACE_H_

#include <interface/interface_includes.h>

// forward declaration of derived class for this interface
class WiFiClientInterface;
class WiFiClient;
class IPAddress;

/**
 * iWiFiClientInterface class
 */
class iWiFiClientInterface
{

public:
  /**
   * iWiFiClientInterface constructor.
   */
  iWiFiClientInterface() {}
  /**
   * iWiFiClientInterface destructor.
   */
  virtual ~iWiFiClientInterface() {}

  virtual uint8_t status() = 0;
  virtual int connect(const char *host, uint16_t port) = 0;
  virtual int connect(const String &host, uint16_t port) = 0;

  virtual size_t write(uint8_t byte) = 0;
  virtual void write(const char *str) = 0;
  virtual size_t write(const uint8_t *buf, size_t size) = 0;
  virtual size_t writeln() = 0;
  virtual size_t writeln(uint8_t byte) = 0;
  virtual size_t writeln(const char *str) = 0;

  virtual int available() = 0;
  virtual int read() = 0;
  virtual int read(uint8_t *buf, size_t size) = 0;
  virtual int peek() = 0;
  virtual size_t peekBytes(uint8_t *buffer, size_t length) = 0;
  virtual bool flush(uint32_t maxWaitMs = 0) = 0;
  virtual bool stop(uint32_t maxWaitMs = 0) = 0;
  virtual uint8_t connected() = 0;
  virtual void setTimeout(uint32_t timeout) = 0;

  virtual IPAddress remoteIP() = 0;
  virtual uint16_t remotePort() = 0;
  virtual IPAddress localIP() = 0;
  virtual uint16_t localPort() = 0;
  virtual void stopAll() = 0;
  virtual int availableForWrite() = 0;

  virtual WiFiClient *getWiFiClient() = 0;
};

// derived class must define this
extern WiFiClientInterface __i_wifi_client;

#endif
