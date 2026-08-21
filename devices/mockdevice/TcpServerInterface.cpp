/**************************** Tcp Server Interface ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include "TcpServerInterface.h"
#include "TcpClientInterface.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

TcpServerInterface::TcpServerInterface() : m_socket(-1),
                                           m_port(0),
                                           m_timeout(0),
                                           m_onaccept(nullptr),
                                           m_onacceptarg(nullptr)
{
}

TcpServerInterface::~TcpServerInterface()
{
    close();
}

/**
 * where a well known port lands when the host will not give it up. zero for a
 * port that has no shadow, so the failure is reported rather than hidden.
 */
uint16_t TcpServerInterface::shadowPort(uint16_t port)
{
    if (0 == port || port >= MOCKDEVICE_TCP_SHADOW_PORT_BASE)
    {
        return 0;
    }

    return (uint16_t)(MOCKDEVICE_TCP_SHADOW_PORT_BASE + port);
}

int32_t TcpServerInterface::begin(uint16_t port)
{
    close();

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0)
    {
        return -1;
    }

    int reuse = 1;
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(m_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        // a board owns its whole port space; a host process does not. where the
        // kernel reserves the port or something else already holds it, the
        // listener moves to the shadow range so the service still comes up and
        // reports where it landed.
        uint16_t shadow = shadowPort(port);
        if (0 == shadow)
        {
            ::close(m_socket);
            m_socket = -1;
            return -1;
        }

        addr.sin_port = htons(shadow);
        if (bind(m_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            ::close(m_socket);
            m_socket = -1;
            return -1;
        }
    }

    if (listen(m_socket, 4) < 0)
    {
        ::close(m_socket);
        m_socket = -1;
        return -1;
    }

    socklen_t len = sizeof(addr);
    if (getsockname(m_socket, (struct sockaddr *)&addr, &len) == 0)
    {
        m_port = ntohs(addr.sin_port);
    }
    else
    {
        m_port = port;
    }

    int flags = fcntl(m_socket, F_GETFL, 0);
    if (flags >= 0)
    {
        fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
    }

    return 0;
}

bool TcpServerInterface::hasClient() const
{
    if (m_socket < 0)
    {
        return false;
    }

    struct pollfd pfd;
    pfd.fd = m_socket;
    pfd.events = POLLIN;
    pfd.revents = 0;

    return poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN);
}

iClientInterface *TcpServerInterface::accept()
{
    if (m_socket < 0)
    {
        return nullptr;
    }

    int clientfd = ::accept(m_socket, nullptr, nullptr);
    if (clientfd < 0)
    {
        return nullptr;
    }

    TcpClientInterface *client = pdiutil::safe_new<TcpClientInterface>(clientfd);
    if (nullptr == client)
    {
        ::close(clientfd);
        return nullptr;
    }

    if (m_timeout > 0)
    {
        client->setTimeout(m_timeout);
    }

    if (nullptr != m_onaccept)
    {
        m_onaccept(m_onacceptarg);
    }

    return client;
}

void TcpServerInterface::setTimeout(uint32_t timeout_ms)
{
    m_timeout = timeout_ms;
}

void TcpServerInterface::close()
{
    if (m_socket >= 0)
    {
        ::close(m_socket);
        m_socket = -1;
    }
    m_port = 0;
}

void TcpServerInterface::setOnAcceptClientEventCallback(CallBackVoidPointerArgFn callbk, void *arg)
{
    m_onaccept = callbk;
    m_onacceptarg = arg;
}

uint16_t TcpServerInterface::getBoundPort() const
{
    return m_port;
}
