/************************* WiFi Client Interface *******************************
This file is part of the Ewings Esp Stack.

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
 * status
 */
uint8_t WiFiClientInterface::status()
{
  return this->m_wifi_client.status();
}

/**
 * connect
 */
int WiFiClientInterface::connect(const char *host, uint16_t port)
{
  return this->m_wifi_client.connect(host, port);
}

/**
 * connect
 */
int WiFiClientInterface::connect(const String &host, uint16_t port)
{
  return this->m_wifi_client.connect(host, port);
}

/**
 * write
 */
size_t WiFiClientInterface::write(uint8_t byte)
{
  return this->m_wifi_client.write(byte);
}

/**
 * write
 */
void WiFiClientInterface::write(const char *str)
{
  this->m_wifi_client.write((const uint8_t *)str, strlen(str));
}

/**
 * write
 */
size_t WiFiClientInterface::write(const uint8_t *buf, size_t size)
{
  return this->m_wifi_client.write(buf, size);
}

/**
 * writeln
 */
size_t WiFiClientInterface::writeln()
{
  return println();
}

/**
 * writeln
 */
size_t WiFiClientInterface::writeln(uint8_t byte)
{
  // size_t l = write(byte);
  // print("\r\n");
  // return l;
  return println(byte);
}

/**
 * writeln
 */
size_t WiFiClientInterface::writeln(const char *str)
{
  // size_t l = write(str);
  // print("\r\n");
  // return l;
  return println(str);
}

/**
 * available
 */
int WiFiClientInterface::available()
{
  return this->m_wifi_client.available();
}

/**
 * read
 */
int WiFiClientInterface::read()
{
  return this->m_wifi_client.read();
}

/**
 * read
 */
int WiFiClientInterface::read(uint8_t *buf, size_t size)
{
  return this->m_wifi_client.read(buf, size);
}

/**
 * peek
 */
int WiFiClientInterface::peek()
{
  return this->m_wifi_client.peek();
}

/**
 * peekBytes
 */
size_t WiFiClientInterface::peekBytes(uint8_t *buffer, size_t length)
{
  return this->m_wifi_client.peekBytes(buffer, length);
}

/**
 * flush
 */
bool WiFiClientInterface::flush(uint32_t maxWaitMs)
{
  return this->m_wifi_client.flush(maxWaitMs);
}

/**
 * stop
 */
bool WiFiClientInterface::stop(uint32_t maxWaitMs)
{
  return this->m_wifi_client.stop(maxWaitMs);
}

/**
 * connected
 */
uint8_t WiFiClientInterface::connected()
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
 * remoteIP
 */
IPAddress WiFiClientInterface::remoteIP()
{
  return this->m_wifi_client.remoteIP();
}

/**
 * localIP
 */
IPAddress WiFiClientInterface::localIP()
{
  return this->m_wifi_client.localIP();
}

/**
 * remotePort
 */
uint16_t WiFiClientInterface::remotePort()
{
  return this->m_wifi_client.remotePort();
}

/**
 * localPort
 */
uint16_t WiFiClientInterface::localPort()
{
  return this->m_wifi_client.localPort();
}

/**
 * availableForWrite
 */
int WiFiClientInterface::availableForWrite()
{
  return this->m_wifi_client.availableForWrite();
}

/**
 * stopAll
 */
void WiFiClientInterface::stopAll()
{
  WiFiClient::stopAll();
}

WiFiClientInterface __i_wifi_client;
