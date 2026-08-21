/******************************* Udp Interface ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include "UdpInterface.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

UdpInterface *UdpInterface::s_open[MOCKDEVICE_UDP_MAX_SOCKETS] = {nullptr};

UdpInterface::UdpInterface() : m_socket(-1), m_port(0), m_onpacket(nullptr)
{
}

UdpInterface::~UdpInterface()
{
    close();
}

void UdpInterface::track()
{
    for (uint8_t i = 0; i < MOCKDEVICE_UDP_MAX_SOCKETS; i++)
    {
        if (this == s_open[i])
        {
            return;
        }
    }

    for (uint8_t i = 0; i < MOCKDEVICE_UDP_MAX_SOCKETS; i++)
    {
        if (nullptr == s_open[i])
        {
            s_open[i] = this;
            return;
        }
    }
}

void UdpInterface::untrack()
{
    for (uint8_t i = 0; i < MOCKDEVICE_UDP_MAX_SOCKETS; i++)
    {
        if (this == s_open[i])
        {
            s_open[i] = nullptr;
        }
    }
}

bool UdpInterface::begin(uint16_t local_port)
{
    close();

    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket < 0)
    {
        return false;
    }

    int reuse = 1;
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(local_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(m_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        ::close(m_socket);
        m_socket = -1;
        return false;
    }

    socklen_t len = sizeof(addr);
    m_port = (getsockname(m_socket, (struct sockaddr *)&addr, &len) == 0) ? ntohs(addr.sin_port)
                                                                         : local_port;

    int flags = fcntl(m_socket, F_GETFL, 0);
    if (flags >= 0)
    {
        fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
    }

    track();
    return true;
}

bool UdpInterface::joinMulticastGroup(const ipaddress_t &group)
{
    if (m_socket < 0)
    {
        return false;
    }

    struct ip_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    mreq.imr_multiaddr.s_addr = htonl(((uint32_t)group.ip4[0] << 24) | ((uint32_t)group.ip4[1] << 16) |
                                      ((uint32_t)group.ip4[2] << 8) | (uint32_t)group.ip4[3]);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    return setsockopt(m_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == 0;
}

int32_t UdpInterface::send(const uint8_t *data, uint16_t len, const ipaddress_t &dst, uint16_t dst_port)
{
    if (nullptr == data || 0 == len || m_socket < 0)
    {
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(dst_port);
    addr.sin_addr.s_addr = htonl(((uint32_t)dst.ip4[0] << 24) | ((uint32_t)dst.ip4[1] << 16) |
                                 ((uint32_t)dst.ip4[2] << 8) | (uint32_t)dst.ip4[3]);

    ssize_t sent = sendto(m_socket, data, len, 0, (struct sockaddr *)&addr, sizeof(addr));
    return (sent < 0) ? -1 : (int32_t)sent;
}

void UdpInterface::setOnPacketCallback(CallBackVoidPointerArgFn callbk)
{
    m_onpacket = callbk;
}

void UdpInterface::close()
{
    untrack();

    if (m_socket >= 0)
    {
        ::close(m_socket);
        m_socket = -1;
    }

    m_port = 0;
}

uint16_t UdpInterface::getBoundPort() const
{
    return m_port;
}

uint16_t UdpInterface::service()
{
    if (m_socket < 0 || !m_onpacket)
    {
        return 0;
    }

    uint8_t buffer[MOCKDEVICE_UDP_RX_BUFFER];
    uint16_t delivered = 0;

    while (true)
    {
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        memset(&from, 0, sizeof(from));

        ssize_t n = recvfrom(m_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&from, &fromlen);
        if (n <= 0)
        {
            break;
        }

        uint32_t raw = ntohl(from.sin_addr.s_addr);

        udp_packet_t packet;
        packet.m_data = buffer;
        packet.m_len = (uint16_t)n;
        packet.m_src_ip = ipaddress_t((uint8_t)((raw >> 24) & 0xFF), (uint8_t)((raw >> 16) & 0xFF),
                                      (uint8_t)((raw >> 8) & 0xFF), (uint8_t)(raw & 0xFF));
        packet.m_src_port = ntohs(from.sin_port);

        m_onpacket(&packet);
        delivered++;
    }

    return delivered;
}

uint16_t UdpInterface::serviceAll()
{
    uint16_t delivered = 0;

    for (uint8_t i = 0; i < MOCKDEVICE_UDP_MAX_SOCKETS; i++)
    {
        if (nullptr != s_open[i])
        {
            delivered += s_open[i]->service();
        }
    }

    return delivered;
}
