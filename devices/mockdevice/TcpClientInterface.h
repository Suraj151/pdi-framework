/**************************** Tcp Client Interface ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#ifndef _MOCKDEVICE_TCP_CLIENT_INTERFACE_H_
#define _MOCKDEVICE_TCP_CLIENT_INTERFACE_H_

#include "mockdevice.h"
#include <interface/pdi/middlewares/iClientInterface.h>

/**
 * @class TcpClientInterface
 * @brief A TCP client over a host socket.
 *
 * The socket is non blocking, so a read returns what has arrived rather than
 * waiting, matching how the lwIP backed ports behave inside the serve loop.
 */
class TcpClientInterface : public iTcpClientInterface
{
public:
    TcpClientInterface();

    /**
     * @brief Adopt an already accepted socket. The client owns it from here.
     */
    explicit TcpClientInterface(int socketfd);

    ~TcpClientInterface();

    // connect / disconnect api
    int16_t connect(const uint8_t *host, uint16_t port) override;
    int16_t disconnect() override;
    int16_t close() override;
    int8_t connected() override;

    // overriding part of the write and read sets would otherwise hide the
    // char, numeric and padded forms the base supplies
    using iTcpClientInterface::read;
    using iTcpClientInterface::write;

    // data sending api
    int32_t write(uint8_t c) override;
    int32_t write(const uint8_t *c_str) override;
    int32_t write(const uint8_t *c_str, uint32_t size) override;
    int32_t write_ro(const char *c_str) override;

    // data receiving api
    uint8_t read() override;
    int32_t read(uint8_t *buf, uint32_t size) override;

    // useful api
    int32_t available() override;
    bool availableforwrite(uint32_t size) override;
    void setTimeout(uint32_t timeout) override;
    void flush(int16_t flushtype = FLUSH_TX) override;

    // address api
    ipaddress_t getLocalIp() const override;
    uint16_t getLocalPort() const override;
    ipaddress_t getRemoteIp() const override;
    uint16_t getRemotePort() const override;

    bool setKeepAlive(uint16_t idleTime, uint16_t interval, uint16_t count) override;
    void setNoDelay(bool noDelay) override;

private:
    int m_socket;
    uint32_t m_timeout;
    int16_t m_peeked;

    bool fillPeek();
    void closeSocket();
};

#endif // _MOCKDEVICE_TCP_CLIENT_INTERFACE_H_
