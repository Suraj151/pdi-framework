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
#include <interface/pdi/middlewares/iWiFiClientInterface.h>

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

  // connect/disconnect api
  int16_t connect(const uint8_t *host, uint16_t port) override;
  int16_t disconnect() override;

  // data sending api
  uint32_t write(uint8_t c) override;
  uint32_t write(const uint8_t *c_str) override;
  uint32_t write(const uint8_t *c_str, uint32_t size) override;

  // received data read api
  uint8_t read() override;
  uint32_t read(uint8_t *buf, uint32_t size) override;

  // useful api
  int32_t available() override;
  int8_t connected() override;
  void setTimeout(uint32_t timeout) override;
  void flush() override;

  WiFiClient *getWiFiClient() override { return &this->m_wifi_client; }

  using Print::write;

private:
  /**
   * @var	WiFiClient  m_wifi_client
   */
  WiFiClient m_wifi_client;
};

#endif
