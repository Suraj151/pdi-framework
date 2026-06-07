/************************* TCP Server Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st May 2025
******************************************************************************/
#include "TcpServerInterface.h"
#include "DeviceControlInterface.h"

#define TCP_GUARD_BEGIN
#define TCP_GUARD_END
// #define TCP_GUARD_BEGIN \
//     bool _pdi_need_lock = !sys_thread_tcpip(LWIP_CORE_LOCK_QUERY_HOLDER); \
//     if (_pdi_need_lock) { LOCK_LWIP_TCPIP_CORE; }
// #define TCP_GUARD_END \
//     if (_pdi_need_lock) { UNLOCK_LWIP_TCPIP_CORE; }


TcpServerInterface::TcpServerInterface():
    m_serverPcb(nullptr), 
    m_clientPcb(nullptr), 
    m_timeout(30000), 
    m_hasClient(false),
    m_onAcceptCallbk(nullptr),
    m_onAcceptCallbkArg(nullptr) {}

TcpServerInterface::~TcpServerInterface() {
    close();
}

/**
 * @brief begines listening on the specified port.
 * @param port The port number to listen on.
 * @return 0 on success, negative error code on failure.
 */
int32_t TcpServerInterface::begin(uint16_t port) {
    close();

    TCP_GUARD_BEGIN
    m_serverPcb = tcp_new();
    if (!m_serverPcb) {
        TCP_GUARD_END
        return -99;
    }

    err_t err = tcp_bind(m_serverPcb, IP_ADDR_ANY, port);
    if (err != ERR_OK) {
        tcp_close(m_serverPcb);
        m_serverPcb = nullptr;
        TCP_GUARD_END
        return err;
    }

    m_serverPcb = tcp_listen(m_serverPcb);
    if (!m_serverPcb) {
        TCP_GUARD_END
        return -99;
    }

    tcp_arg(m_serverPcb, this);
    tcp_accept(m_serverPcb, &TcpServerInterface::onAccept);
    m_hasClient = false;
    m_clientPcb = nullptr;
    TCP_GUARD_END
    return 0;
}

/**
 * @brief Closes the server and all connected clients.
 */
void TcpServerInterface::close() {
    TCP_GUARD_BEGIN
    if (m_clientPcb) {
        tcp_arg(m_clientPcb, nullptr);
        tcp_sent(m_clientPcb, nullptr);
        tcp_recv(m_clientPcb, nullptr);
        tcp_err(m_clientPcb, nullptr);
        tcp_close(m_clientPcb);
        m_clientPcb = nullptr;
    }
    if (m_serverPcb) {
        tcp_arg(m_serverPcb, nullptr);
        tcp_accept(m_serverPcb, nullptr);
        tcp_close(m_serverPcb);
        m_serverPcb = nullptr;
    }
    m_hasClient = false;
    TCP_GUARD_END
}

/**
 * @brief Set callback event for when a new client connection is accepted.
 */
void TcpServerInterface::setOnAcceptClientEventCallback(CallBackVoidPointerArgFn callbk, void* arg) {
    m_onAcceptCallbk = callbk;
    m_onAcceptCallbkArg = arg;
}

/**
 * @brief Check if there is a client connected.
 * @return true if a client is connected, false otherwise.
 */
bool TcpServerInterface::hasClient() const {
    TCP_GUARD_BEGIN
    bool v = m_hasClient && m_clientPcb;
    TCP_GUARD_END
    return v;
}

/**
 * @brief Accepts a new client connection.
 * @return Pointer to the accepted client interface, or nullptr if no client is connected.
 */
iClientInterface* TcpServerInterface::accept() {
    TCP_GUARD_BEGIN
    if (m_hasClient && m_clientPcb) {
        m_hasClient = false;
        struct tcp_pcb* pcb = m_clientPcb;
        m_clientPcb = nullptr;
        TCP_GUARD_END
        return new TcpClientInterface(pcb);
    }
    TCP_GUARD_END
    return nullptr;
}

/**
 * @brief Set the timeout for client connections.
 * @param timeout_ms The timeout value in milliseconds.
 */
void TcpServerInterface::setTimeout(uint32_t timeout_ms) {
    m_timeout = timeout_ms;
}

/**
 * @brief Callback for when a new client connection is accepted.
 * @param arg User-defined argument (this instance).
 * @param newpcb Pointer to the new TCP protocol control block.
 * @param err Error code.
 * @return Error code.
 */
err_t TcpServerInterface::onAccept(void* arg, struct tcp_pcb* newpcb, err_t err) {
    TcpServerInterface* server = static_cast<TcpServerInterface*>(arg);
    if (!server || !newpcb) return ERR_VAL;

    // Only allow one client at a time for simplicity
    if (server->m_clientPcb) {
        // tcp_abort(newpcb);
        // return ERR_ABRT;
        tcp_abort(server->m_clientPcb);
    }

    server->m_clientPcb = newpcb;
    server->m_hasClient = true;

    // Optionally set keepalive or other options here
    // newpcb->so_options |= SOF_KEEPALIVE;

    // trigger the callback if registered
    if( server->m_onAcceptCallbk ){
        server->m_onAcceptCallbk(server->m_onAcceptCallbkArg);
    }

    return ERR_OK;
}

