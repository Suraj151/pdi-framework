/************************* WiFi Client Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "WiFiClientInterface.h"

/**
 * WiFiClientInterface constructor.
 */
WiFiClientInterface::WiFiClientInterface()
{
}

/**
 * WiFiClientInterface destructor.
 */
WiFiClientInterface::~WiFiClientInterface()
{
}

/**
 * connect
 */
int16_t WiFiClientInterface::connect(const uint8_t *host, uint16_t port)
{
  return this->m_wifi_client.connect((const char*)host, port);
}

/**
 * disconnect
 */
int16_t WiFiClientInterface::disconnect()
{
  return this->m_wifi_client.stop(0);
}

/**
 * write
 */
int32_t WiFiClientInterface::write(uint8_t c)
{
  return (int32_t)this->m_wifi_client.write(c);
}

/**
 * write
 */
int32_t WiFiClientInterface::write(const uint8_t *c_str)
{
  return this->m_wifi_client.write(c_str, strlen((const char*)c_str));
}

/**
 * write
 */
int32_t WiFiClientInterface::write(const uint8_t *c_str, uint32_t size)
{
  return this->m_wifi_client.write(c_str, size);
}

/**
 * read
 */
uint8_t WiFiClientInterface::read()
{
  return this->m_wifi_client.read();
}

/**
 * read
 */
int32_t WiFiClientInterface::read(uint8_t *buf, uint32_t size)
{
  return this->m_wifi_client.read(buf, size);
}

/**
 * available
 */
int32_t WiFiClientInterface::available()
{
  return this->m_wifi_client.available();
}

/**
 * connected
 */
int8_t WiFiClientInterface::connected()
{
  return this->m_wifi_client.connected();
}

/**
 * setTimeout
 */
void WiFiClientInterface::setTimeout(uint32_t timeout)
{
  this->m_wifi_client.setTimeout(timeout);
}

/**
 * flush
 */
void WiFiClientInterface::flush()
{
  this->m_wifi_client.flush();
}

WiFiClientInterface __i_wifi_client;
