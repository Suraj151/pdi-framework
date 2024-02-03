/************************** WiFi Client Interface ******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WIFI_CLIENT_INTERFACE_H_
#define _WIFI_CLIENT_INTERFACE_H_

#include "esp8266.h"
#include <interface/pdi/iWiFiClientInterface.h>

/**
 * WiFiClientInterface class
 */
class WiFiClientInterface : public iWiFiClientInterface, public Print
{

public:
  /**
   * WiFiClientInterface constructor.
   */
  WiFiClientInterface();
  /**
   * WiFiClientInterface destructor.
   */
  ~WiFiClientInterface();

  uint8_t status() override;
  int connect(const char *host, uint16_t port) override;
  int connect(const String &host, uint16_t port) override;

  size_t write(uint8_t byte) override;
  void write(const char *str) override;
  size_t write(const uint8_t *buf, size_t size) override;
  size_t writeln() override;
  size_t writeln(uint8_t byte) override;
  size_t writeln(const char *str) override;

  int available() override;
  int read() override;
  int read(uint8_t *buf, size_t size) override;
  int peek() override;
  size_t peekBytes(uint8_t *buffer, size_t length) override;
  bool flush(uint32_t maxWaitMs = 0) override;
  bool stop(uint32_t maxWaitMs = 0) override;
  uint8_t connected() override;
  void setTimeout(uint32_t timeout) override;

  IPAddress remoteIP() override;
  uint16_t remotePort() override;
  IPAddress localIP() override;
  uint16_t localPort() override;
  int availableForWrite() override;
  void stopAll() override;

  WiFiClient *getWiFiClient() override { return &this->m_wifi_client; }

  using Print::write;

private:
  /**
   * @var	WiFiClient  m_wifi_client
   */
  WiFiClient m_wifi_client;
};

#endif
