/**************************** Tcp Server Interface ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#ifndef _MOCKDEVICE_TCP_SERVER_INTERFACE_H_
#define _MOCKDEVICE_TCP_SERVER_INTERFACE_H_

#include "mockdevice.h"
#include <interface/pdi/middlewares/iServerInterface.h>

/**
 * a port the host refuses is retried at this base plus the port asked for, so
 * 22 becomes 10022 and 80 becomes 10080
 */
#ifndef MOCKDEVICE_TCP_SHADOW_PORT_BASE
#define MOCKDEVICE_TCP_SHADOW_PORT_BASE 10000
#endif

/**
 * @class TcpServerInterface
 * @brief A listening socket that hands out TcpClientInterface instances.
 *
 * The listener is non blocking, so hasClient and accept answer immediately and
 * can be polled from the serve loop.
 */
class TcpServerInterface : public iTcpServerInterface
{
public:
    TcpServerInterface();
    ~TcpServerInterface();

    int32_t begin(uint16_t port) override;
    bool hasClient() const override;
    iClientInterface *accept() override;
    void setTimeout(uint32_t timeout_ms) override;
    void close() override;
    void setOnAcceptClientEventCallback(CallBackVoidPointerArgFn callbk, void *arg = nullptr) override;

    /**
     * @brief Port the listener actually bound to. Asking for port 0 lets the
     *        host choose a free one, which keeps parallel runs from colliding.
     */
    uint16_t getBoundPort() const;

    /**
     * @brief Where a port the host refuses is retried, or 0 if it has no
     *        shadow. A caller reaching the listener uses getBoundPort.
     */
    static uint16_t shadowPort(uint16_t port);

private:
    int m_socket;
    uint16_t m_port;
    uint32_t m_timeout;
    CallBackVoidPointerArgFn m_onaccept;
    void *m_onacceptarg;
};

#endif // _MOCKDEVICE_TCP_SERVER_INTERFACE_H_
