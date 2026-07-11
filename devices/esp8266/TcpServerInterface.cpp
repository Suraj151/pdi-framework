/************************* TCP Server Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st May 2025
******************************************************************************/
#include "TcpServerInterface.h"
#include "DeviceControlInterface.h"


TcpServerInterface::TcpServerInterface():
    m_serverPcb(nullptr),
    m_clientPcb(nullptr),
    m_timeout(30000),
    m_pendingSince(0),
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

    m_serverPcb = tcp_new();
    if (!m_serverPcb) return -99;

    err_t err = tcp_bind(m_serverPcb, IP_ADDR_ANY, port);
    if (err != ERR_OK) {
        tcp_close(m_serverPcb);
        m_serverPcb = nullptr;
        return err;
    }

    m_serverPcb = tcp_listen_with_backlog(m_serverPcb, 1);//tcp_listen(m_serverPcb);
    if (!m_serverPcb) return -99;

    tcp_arg(m_serverPcb, this);
    tcp_accept(m_serverPcb, &TcpServerInterface::onAccept);
    m_hasClient = false;
    m_clientPcb = nullptr;
    return 0;
}

/**
 * @brief Closes the server and all connected clients.
 */
void TcpServerInterface::close() {
    if (m_clientPcb) {
        tcp_arg(m_clientPcb, nullptr);
        tcp_sent(m_clientPcb, nullptr);
        tcp_recv(m_clientPcb, nullptr);
        tcp_poll(m_clientPcb, nullptr, 0);
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
    return m_hasClient && m_clientPcb;
}

/**
 * @brief Accepts a new client connection.
 * @return Pointer to the accepted client interface, or nullptr if no client is connected.
 */
iClientInterface* TcpServerInterface::accept() {
    if (!hasClient()) return nullptr;

    struct tcp_pcb* pcb = m_clientPcb;
    m_clientPcb = nullptr;
    m_hasClient = false;
    m_pendingSince = 0;

    tcp_arg (pcb, nullptr);
    tcp_recv(pcb, nullptr);
    tcp_err (pcb, nullptr);
    tcp_poll(pcb, nullptr, 0);

    return new TcpClientInterface(pcb);
}

/**
 * @brief Set the timeout for client connections.
 * @param timeout_ms The timeout value in milliseconds.
 */
void TcpServerInterface::setTimeout(uint32_t timeout_ms) {
    m_timeout = timeout_ms;
}

void TcpServerInterface::onPendingError(void* arg, err_t /*err*/) {
    TcpServerInterface* server = static_cast<TcpServerInterface*>(arg);
    if (!server) return;
    server->m_clientPcb = nullptr;
    server->m_hasClient = false;
    server->m_pendingSince = 0;
}

err_t TcpServerInterface::onPendingRecv(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t /*err*/) {
    TcpServerInterface* server = static_cast<TcpServerInterface*>(arg);
    if (!server || !pcb) return ERR_ARG;

    if (p == nullptr) {
        if (server->m_clientPcb == pcb) {
            server->m_clientPcb = nullptr;
            server->m_hasClient = false;
            server->m_pendingSince = 0;
        }
        tcp_arg (pcb, nullptr);
        tcp_recv(pcb, nullptr);
        tcp_err (pcb, nullptr);
        tcp_poll(pcb, nullptr, 0);
        tcp_close(pcb);
        return ERR_OK;
    }

    return ERR_MEM;
}

err_t TcpServerInterface::onPendingPoll(void* arg, struct tcp_pcb* pcb) {
    TcpServerInterface* server = static_cast<TcpServerInterface*>(arg);
    if (!server || !pcb) return ERR_ABRT;

    if (server->m_clientPcb == pcb &&
        server->m_timeout > 0 &&
        (millis() - server->m_pendingSince) > server->m_timeout) {

        server->m_clientPcb = nullptr;
        server->m_hasClient = false;
        server->m_pendingSince = 0;
        tcp_arg (pcb, nullptr);
        tcp_recv(pcb, nullptr);
        tcp_err (pcb, nullptr);
        tcp_poll(pcb, nullptr, 0);
        tcp_abort(pcb);
        return ERR_ABRT;
    }
    return ERR_OK;
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

    if (server->m_clientPcb) {
        struct tcp_pcb* old = server->m_clientPcb;
        server->m_clientPcb = nullptr;
        server->m_hasClient = false;
        server->m_pendingSince = 0;
        tcp_arg (old, nullptr);
        tcp_recv(old, nullptr);
        tcp_err (old, nullptr);
        tcp_poll(old, nullptr, 0);
        tcp_abort(old);
    }

    server->m_clientPcb    = newpcb;
    server->m_hasClient    = true;
    server->m_pendingSince = millis();

    tcp_arg (newpcb, server);
    tcp_err (newpcb, &TcpServerInterface::onPendingError);
    tcp_recv(newpcb, &TcpServerInterface::onPendingRecv);
    tcp_poll(newpcb, &TcpServerInterface::onPendingPoll, 4);

    if( server->m_onAcceptCallbk ){
        server->m_onAcceptCallbk(server->m_onAcceptCallbkArg);
    }

    return ERR_OK;
}

