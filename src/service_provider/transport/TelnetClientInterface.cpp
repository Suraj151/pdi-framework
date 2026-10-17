/************************* Telnet Client interface ***************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 9th Oct 2026
******************************************************************************/

#include "TelnetClientInterface.h"

#ifdef ENABLE_TELNET_SERVICE

/**
 * Wraps an accepted tcp client, taking ownership of it.
 */
TelnetClientInterface::TelnetClientInterface(iTcpClientInterface *client) : m_tcpClient(client)
{
}

/**
 * Closes and frees the tcp client underneath.
 */
TelnetClientInterface::~TelnetClientInterface()
{
    if (nullptr != m_tcpClient) {
        m_tcpClient->close();
        pdiutil::safe_delete(m_tcpClient);
    }
}

int16_t TelnetClientInterface::connect(const uint8_t *host, uint16_t port)
{
    return nullptr != m_tcpClient ? m_tcpClient->connect(host, port) : -1;
}

int16_t TelnetClientInterface::disconnect()
{
    return nullptr != m_tcpClient ? m_tcpClient->disconnect() : -1;
}

int16_t TelnetClientInterface::close()
{
    return nullptr != m_tcpClient ? m_tcpClient->close() : -1;
}

int8_t TelnetClientInterface::connected()
{
    return nullptr != m_tcpClient ? m_tcpClient->connected() : 0;
}

/**
 * Sends the bytes with every 0xFF doubled, reporting the caller's byte count.
 */
int32_t TelnetClientInterface::write(const uint8_t *c_str, uint32_t size)
{
    if (nullptr == m_tcpClient || nullptr == c_str) {
        return 0;
    }

    uint32_t start = 0;
    for (uint32_t i = 0; i < size; i++) {
        if (0xFF != c_str[i]) {
            continue;
        }
        if (i + 1 > start) {
            m_tcpClient->write(c_str + start, i + 1 - start);
        }
        m_tcpClient->write((uint8_t)0xFF);
        start = i + 1;
    }
    if (size > start) {
        m_tcpClient->write(c_str + start, size - start);
    }
    return (int32_t)size;
}

/**
 * Sends one byte, doubled when it is 0xFF.
 */
int32_t TelnetClientInterface::write(uint8_t c)
{
    return write(&c, 1);
}

/**
 * Sends a nul terminated string with every 0xFF doubled.
 */
int32_t TelnetClientInterface::write(const uint8_t *c_str)
{
    return nullptr != c_str ? write(c_str, (uint32_t)strlen((const char *)c_str)) : 0;
}

int32_t TelnetClientInterface::write_ro(const char *c_str)
{
    return nullptr != m_tcpClient ? m_tcpClient->write_ro(c_str) : 0;
}

int32_t TelnetClientInterface::read(uint8_t *buffer, uint32_t size)
{
    return nullptr != m_tcpClient ? m_tcpClient->read(buffer, size) : 0;
}

uint8_t TelnetClientInterface::read()
{
    return nullptr != m_tcpClient ? m_tcpClient->read() : 0;
}

int32_t TelnetClientInterface::available()
{
    return nullptr != m_tcpClient ? m_tcpClient->available() : 0;
}

bool TelnetClientInterface::availableforwrite(uint32_t size)
{
    return nullptr != m_tcpClient ? m_tcpClient->availableforwrite(size) : false;
}

void TelnetClientInterface::flush(int16_t flushtype)
{
    if (nullptr != m_tcpClient) {
        m_tcpClient->flush(flushtype);
    }
}

int32_t TelnetClientInterface::commit()
{
    return nullptr != m_tcpClient ? m_tcpClient->commit() : 0;
}

ipaddress_t TelnetClientInterface::getLocalIp() const
{
    return nullptr != m_tcpClient ? m_tcpClient->getLocalIp() : ipaddress_t();
}

uint16_t TelnetClientInterface::getLocalPort() const
{
    return nullptr != m_tcpClient ? m_tcpClient->getLocalPort() : 0;
}

ipaddress_t TelnetClientInterface::getRemoteIp() const
{
    return nullptr != m_tcpClient ? m_tcpClient->getRemoteIp() : ipaddress_t();
}

uint16_t TelnetClientInterface::getRemotePort() const
{
    return nullptr != m_tcpClient ? m_tcpClient->getRemotePort() : 0;
}

bool TelnetClientInterface::setKeepAlive(uint16_t idleTime, uint16_t interval, uint16_t count)
{
    return nullptr != m_tcpClient ? m_tcpClient->setKeepAlive(idleTime, interval, count) : false;
}

void TelnetClientInterface::setNoDelay(bool noDelay)
{
    if (nullptr != m_tcpClient) {
        m_tcpClient->setNoDelay(noDelay);
    }
}

void TelnetClientInterface::setTimeout(uint32_t timeout)
{
    if (nullptr != m_tcpClient) {
        m_tcpClient->setTimeout(timeout);
    }
}

#endif
