/************************* TCP Server Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st May 2025
******************************************************************************/
#include "TcpServerInterface.h"


TcpServerInterface::TcpServerInterface()
    : m_serverPcb(nullptr), m_clientPcb(nullptr), m_timeout(30000), m_hasClient(false) {}

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

    m_serverPcb = tcp_listen(m_serverPcb);
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
    if (hasClient()) {
        m_hasClient = false;
        TcpClientInterface *client = new TcpClientInterface(m_clientPcb);
        m_clientPcb = nullptr; // Clear the client PCB after accepting
        return client;
    }
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
        tcp_abort(newpcb);
        return ERR_ABRT;
    }

    server->m_clientPcb = newpcb;
    server->m_hasClient = true;

    // Optionally set keepalive or other options here
    // newpcb->so_options |= SOF_KEEPALIVE;

    return ERR_OK;
}

