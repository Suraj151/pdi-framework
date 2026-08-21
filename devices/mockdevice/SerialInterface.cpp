/****************************** Serial Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include "SerialInterface.h"
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>

// initialize instances with respective type or nullptr
iSerialInterface *iSerialInterface::instances[SERIAL_IFACE_MAX] = {
    &__serial_uart  // SERIAL_IFACE_UART
    ,&__serial_uart1 // SERIAL_IFACE_UART1
    ,nullptr        // SERIAL_IFACE_I2C
    ,nullptr        // SERIAL_IFACE_I2C1
    ,nullptr        // SERIAL_IFACE_SPI
    ,nullptr        // SERIAL_IFACE_SPI1
    ,nullptr        // SERIAL_IFACE_CAN
    ,nullptr        // SERIAL_IFACE_CAN1
    ,nullptr        // SERIAL_IFACE_CMD
    ,nullptr        // SERIAL_IFACE_IOT
};

/**
 * UARTSerial constructor.
 */
UARTSerial::UARTSerial(int readfd, int writefd) : m_connected(false),
                                                  m_port(0),
                                                  m_speed(0),
                                                  m_timeout(0),
                                                  m_readfd(readfd),
                                                  m_writefd(writefd),
                                                  m_peeked(-1)
{
}

/**
 * UARTSerial destructor.
 */
UARTSerial::~UARTSerial()
{
}

int16_t UARTSerial::connect(uint16_t port, uint64_t speed)
{
    m_port = port;
    m_speed = speed;
    m_connected = (m_readfd >= 0 || m_writefd >= 0);
    return m_connected ? 0 : -1;
}

int16_t UARTSerial::disconnect()
{
    m_connected = false;
    return 0;
}

int32_t UARTSerial::write(uint8_t c)
{
    return this->write(&c, 1);
}

int32_t UARTSerial::write(const uint8_t *c_str)
{
    if (nullptr == c_str)
    {
        return 0;
    }
    return this->write(c_str, (uint32_t)strlen((const char *)c_str));
}

int32_t UARTSerial::write(const uint8_t *c_str, uint32_t size)
{
    if (nullptr == c_str || 0 == size || m_writefd < 0)
    {
        return 0;
    }

    uint32_t written = 0;
    while (written < size)
    {
        ssize_t n = ::write(m_writefd, c_str + written, size - written);
        if (n <= 0)
        {
            if (n < 0 && EINTR == errno)
            {
                continue;
            }
            break;
        }
        written += (uint32_t)n;
    }

    return (int32_t)written;
}

int32_t UARTSerial::write_ro(const char *c_str)
{
    return this->write((const uint8_t *)c_str);
}

/**
 * pull one byte into the peek slot so available() can answer without consuming
 */
bool UARTSerial::fill_peek()
{
    if (m_peeked >= 0)
    {
        return true;
    }

    if (m_readfd < 0)
    {
        return false;
    }

    struct pollfd pfd;
    pfd.fd = m_readfd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    if (poll(&pfd, 1, 0) <= 0)
    {
        return false;
    }

    uint8_t c = 0;
    ssize_t n = ::read(m_readfd, &c, 1);
    if (n != 1)
    {
        return false;
    }

    m_peeked = (int16_t)c;
    return true;
}

uint8_t UARTSerial::read()
{
    if (!fill_peek())
    {
        return 0;
    }

    uint8_t c = (uint8_t)m_peeked;
    m_peeked = -1;
    return c;
}

int32_t UARTSerial::read(uint8_t *buf, uint32_t size)
{
    if (nullptr == buf || 0 == size)
    {
        return 0;
    }

    uint32_t count = 0;
    while (count < size && fill_peek())
    {
        buf[count++] = (uint8_t)m_peeked;
        m_peeked = -1;
    }

    return (int32_t)count;
}

int32_t UARTSerial::available()
{
    return fill_peek() ? 1 : 0;
}

int8_t UARTSerial::connected()
{
    return m_connected ? 1 : 0;
}

void UARTSerial::setTimeout(uint32_t timeout)
{
    m_timeout = timeout;
}

void UARTSerial::flush(int16_t flushtype)
{
    if (IsFlushTx(flushtype) && m_writefd >= 0)
    {
        fsync(m_writefd);
    }

    // modelled rather than skipped: a discard the host device does not perform
    // is a whole class of defect the host tier cannot see
    if (IsFlushRx(flushtype))
    {
        m_peeked = -1;
        while (available() > 0)
        {
            read();
        }
    }
}

void UARTSerial::setDescriptors(int readfd, int writefd)
{
    m_readfd = readfd;
    m_writefd = writefd;
    m_peeked = -1;
}

UARTSerial __serial_uart(STDIN_FILENO, STDOUT_FILENO);
UARTSerial __serial_uart1(-1, -1);
