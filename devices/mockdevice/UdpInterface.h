/******************************* Udp Interface ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#ifndef _MOCKDEVICE_UDP_INTERFACE_H_
#define _MOCKDEVICE_UDP_INTERFACE_H_

#include "mockdevice.h"
#include <interface/pdi/middlewares/iUdpInterface.h>

#ifndef MOCKDEVICE_UDP_MAX_SOCKETS
#define MOCKDEVICE_UDP_MAX_SOCKETS 4
#endif

#ifndef MOCKDEVICE_UDP_RX_BUFFER
#define MOCKDEVICE_UDP_RX_BUFFER 1024
#endif

/**
 * @class UdpInterface
 * @brief A UDP socket on the host.
 *
 * On the lwIP backed ports the stack delivers datagrams through its own
 * callback. Here nothing calls in on its own, so every open socket is drained
 * from the serve loop through serviceAll, which the device control interface
 * calls out of handleEvents.
 */
class UdpInterface : public iUdpInterface
{
public:
    UdpInterface();
    ~UdpInterface();

    bool begin(uint16_t local_port) override;
    bool joinMulticastGroup(const ipaddress_t &group) override;
    int32_t send(const uint8_t *data, uint16_t len, const ipaddress_t &dst, uint16_t dst_port) override;
    void setOnPacketCallback(CallBackVoidPointerArgFn callbk) override;
    void close() override;

    /**
     * @brief Port the socket bound to, resolved when zero was requested.
     */
    uint16_t getBoundPort() const;

    /**
     * @brief Deliver whatever has arrived on this socket to the callback.
     * @return number of datagrams delivered.
     */
    uint16_t service();

    /**
     * @brief Drain every open socket. Called once per serve pass.
     */
    static uint16_t serviceAll();

private:
    int m_socket;
    uint16_t m_port;
    CallBackVoidPointerArgFn m_onpacket;

    static UdpInterface *s_open[MOCKDEVICE_UDP_MAX_SOCKETS];

    void track();
    void untrack();
};

#endif // _MOCKDEVICE_UDP_INTERFACE_H_
