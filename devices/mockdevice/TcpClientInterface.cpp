/**************************** Tcp Client Interface ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include "TcpClientInterface.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

static void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0)
    {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

static ipaddress_t toIpAddress(const struct sockaddr_in &addr)
{
    uint32_t raw = ntohl(addr.sin_addr.s_addr);
    return ipaddress_t((uint8_t)((raw >> 24) & 0xFF),
                       (uint8_t)((raw >> 16) & 0xFF),
                       (uint8_t)((raw >> 8) & 0xFF),
                       (uint8_t)(raw & 0xFF));
}

TcpClientInterface::TcpClientInterface() : m_socket(-1), m_timeout(0), m_peeked(-1)
{
}

TcpClientInterface::TcpClientInterface(int socketfd) : m_socket(socketfd), m_timeout(0), m_peeked(-1)
{
    if (m_socket >= 0)
    {
        setNonBlocking(m_socket);
    }
}

TcpClientInterface::~TcpClientInterface()
{
    closeSocket();
}

void TcpClientInterface::closeSocket()
{
    if (m_socket >= 0)
    {
        ::close(m_socket);
        m_socket = -1;
    }
    m_peeked = -1;
}

int16_t TcpClientInterface::connect(const uint8_t *host, uint16_t port)
{
    // callers hand this a NUL terminated host string, not four address bytes
    const char *hostname = (const char *)host;

    if (nullptr == hostname || 0 == hostname[0])
    {
        return -1;
    }

    closeSocket();

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, hostname, &addr.sin_addr) != 1)
    {
        struct addrinfo hints;
        struct addrinfo *found = nullptr;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (0 != getaddrinfo(hostname, nullptr, &hints, &found) || nullptr == found)
        {
            return -1;
        }

        addr.sin_addr = ((struct sockaddr_in *)found->ai_addr)->sin_addr;
        freeaddrinfo(found);
    }

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0)
    {
        return -1;
    }

    // connect without blocking and wait only as long as the caller allowed. a
    // blocking connect to an address that never answers stalls the whole
    // single threaded process until the kernel gives up minutes later
    setNonBlocking(m_socket);

    if (::connect(m_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        if (EINPROGRESS != errno)
        {
            closeSocket();
            return -1;
        }

        struct pollfd waiting;
        waiting.fd = m_socket;
        waiting.events = POLLOUT;

        int ready = poll(&waiting, 1, m_timeout > 0 ? (int)m_timeout : 5000);
        if (ready <= 0)
        {
            closeSocket();
            return -1;
        }

        int failure = 0;
        socklen_t length = sizeof(failure);
        if (getsockopt(m_socket, SOL_SOCKET, SO_ERROR, &failure, &length) < 0 || 0 != failure)
        {
            closeSocket();
            return -1;
        }
    }

    return 0;
}

int16_t TcpClientInterface::disconnect()
{
    closeSocket();
    return 0;
}

int16_t TcpClientInterface::close()
{
    return disconnect();
}

int8_t TcpClientInterface::connected()
{
    if (m_socket < 0)
    {
        return 0;
    }

    if (m_peeked >= 0)
    {
        return 1;
    }

    struct pollfd pfd;
    pfd.fd = m_socket;
    pfd.events = POLLIN;
    pfd.revents = 0;

    if (poll(&pfd, 1, 0) > 0)
    {
        if (pfd.revents & (POLLHUP | POLLERR))
        {
            return 0;
        }
        if (pfd.revents & POLLIN)
        {
            // a readable socket with nothing left to read is a closed peer
            uint8_t probe = 0;
            ssize_t n = recv(m_socket, &probe, 1, MSG_PEEK);
            if (0 == n)
            {
                return 0;
            }
        }
    }

    return 1;
}

int32_t TcpClientInterface::write(uint8_t c)
{
    return write(&c, 1);
}

int32_t TcpClientInterface::write(const uint8_t *c_str)
{
    if (nullptr == c_str)
    {
        return 0;
    }
    return write(c_str, (uint32_t)strlen((const char *)c_str));
}

int32_t TcpClientInterface::write(const uint8_t *c_str, uint32_t size)
{
    if (nullptr == c_str || 0 == size || m_socket < 0)
    {
        return 0;
    }

    uint32_t sent = 0;
    while (sent < size)
    {
        ssize_t n = send(m_socket, c_str + sent, size - sent, MSG_NOSIGNAL);
        if (n > 0)
        {
            sent += (uint32_t)n;
            continue;
        }

        if (n < 0 && (EINTR == errno))
        {
            continue;
        }

        if (n < 0 && (EAGAIN == errno || EWOULDBLOCK == errno))
        {
            struct pollfd pfd;
            pfd.fd = m_socket;
            pfd.events = POLLOUT;
            pfd.revents = 0;
            if (poll(&pfd, 1, (int)(m_timeout > 0 ? m_timeout : 1000)) > 0)
            {
                continue;
            }
        }

        break;
    }

    return (int32_t)sent;
}

int32_t TcpClientInterface::write_ro(const char *c_str)
{
    return write((const uint8_t *)c_str);
}

bool TcpClientInterface::fillPeek()
{
    if (m_peeked >= 0)
    {
        return true;
    }

    if (m_socket < 0)
    {
        return false;
    }

    uint8_t c = 0;
    ssize_t n = recv(m_socket, &c, 1, 0);
    if (n != 1)
    {
        return false;
    }

    m_peeked = (int16_t)c;
    return true;
}

uint8_t TcpClientInterface::read()
{
    if (!fillPeek())
    {
        return 0;
    }

    uint8_t c = (uint8_t)m_peeked;
    m_peeked = -1;
    return c;
}

int32_t TcpClientInterface::read(uint8_t *buf, uint32_t size)
{
    if (nullptr == buf || 0 == size || m_socket < 0)
    {
        return 0;
    }

    uint32_t count = 0;

    if (m_peeked >= 0)
    {
        buf[count++] = (uint8_t)m_peeked;
        m_peeked = -1;
    }

    while (count < size)
    {
        ssize_t n = recv(m_socket, buf + count, size - count, 0);
        if (n > 0)
        {
            count += (uint32_t)n;
            continue;
        }
        if (n < 0 && EINTR == errno)
        {
            continue;
        }
        break;
    }

    return (int32_t)count;
}

int32_t TcpClientInterface::available()
{
    if (m_peeked >= 0)
    {
        return 1;
    }

    if (m_socket < 0)
    {
        return 0;
    }

    int pending = 0;
    if (ioctl(m_socket, FIONREAD, &pending) < 0)
    {
        return 0;
    }

    return pending;
}

bool TcpClientInterface::availableforwrite(uint32_t size)
{
    if (m_socket < 0)
    {
        return false;
    }

    struct pollfd pfd;
    pfd.fd = m_socket;
    pfd.events = POLLOUT;
    pfd.revents = 0;

    return poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLOUT);
}

void TcpClientInterface::setTimeout(uint32_t timeout)
{
    m_timeout = timeout;
}

void TcpClientInterface::flush(int16_t flushtype)
{
}

ipaddress_t TcpClientInterface::getLocalIp() const
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));

    if (m_socket < 0 || getsockname(m_socket, (struct sockaddr *)&addr, &len) < 0)
    {
        return ipaddress_t();
    }

    return toIpAddress(addr);
}

uint16_t TcpClientInterface::getLocalPort() const
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));

    if (m_socket < 0 || getsockname(m_socket, (struct sockaddr *)&addr, &len) < 0)
    {
        return 0;
    }

    return ntohs(addr.sin_port);
}

ipaddress_t TcpClientInterface::getRemoteIp() const
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));

    if (m_socket < 0 || getpeername(m_socket, (struct sockaddr *)&addr, &len) < 0)
    {
        return ipaddress_t();
    }

    return toIpAddress(addr);
}

uint16_t TcpClientInterface::getRemotePort() const
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));

    if (m_socket < 0 || getpeername(m_socket, (struct sockaddr *)&addr, &len) < 0)
    {
        return 0;
    }

    return ntohs(addr.sin_port);
}

bool TcpClientInterface::setKeepAlive(uint16_t idleTime, uint16_t interval, uint16_t count)
{
    if (m_socket < 0)
    {
        return false;
    }

    int enable = 1;
    int idle = (int)idleTime;
    int intvl = (int)interval;
    int cnt = (int)count;

    if (setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable)) < 0)
    {
        return false;
    }

    setsockopt(m_socket, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(m_socket, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
    setsockopt(m_socket, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));

    return true;
}

void TcpClientInterface::setNoDelay(bool noDelay)
{
    if (m_socket < 0)
    {
        return;
    }

    int flag = noDelay ? 1 : 0;
    setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
}
