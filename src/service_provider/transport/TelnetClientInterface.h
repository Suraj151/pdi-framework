/************************* Telnet Client interface ***************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 9th Oct 2026
******************************************************************************/

#ifndef _TELNET_CLIENT_INTERFACE_H
#define _TELNET_CLIENT_INTERFACE_H

#include <service_provider/ServiceProvider.h>

#ifdef ENABLE_TELNET_SERVICE

/**
 * A telnet session's connection: everything goes to the tcp client underneath,
 * except that a data byte 0xFF is sent doubled, as telnet requires.
 */
class TelnetClientInterface : public iTcpClientInterface {
public:
    /**
     * Wraps an accepted tcp client, taking ownership of it.
     */
    TelnetClientInterface(iTcpClientInterface *client);

    /**
     * Closes and frees the tcp client underneath.
     */
    ~TelnetClientInterface();

    int16_t connect(const uint8_t *host, uint16_t port) override;
    int16_t disconnect() override;
    int16_t close() override;
    int8_t connected() override;

    /**
     * Sends the bytes with every 0xFF doubled, reporting the caller's byte count.
     */
    int32_t write(const uint8_t *c_str, uint32_t size) override;

    /**
     * Sends one byte, doubled when it is 0xFF.
     */
    int32_t write(uint8_t c) override;

    /**
     * Sends a nul terminated string with every 0xFF doubled.
     */
    int32_t write(const uint8_t *c_str) override;

    int32_t write_ro(const char *c_str) override;
    int32_t read(uint8_t *buffer, uint32_t size) override;
    uint8_t read() override;
    int32_t available() override;
    bool availableforwrite(uint32_t size) override;
    void flush(int16_t flushtype = FLUSH_TX) override;
    int32_t commit() override;

    ipaddress_t getLocalIp() const override;
    uint16_t getLocalPort() const override;
    ipaddress_t getRemoteIp() const override;
    uint16_t getRemotePort() const override;
    bool setKeepAlive(uint16_t idleTime, uint16_t interval, uint16_t count) override;
    void setNoDelay(bool noDelay) override;
    void setTimeout(uint32_t timeout) override;

private:
    iTcpClientInterface *m_tcpClient;
};

#endif

#endif
