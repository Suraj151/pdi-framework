/************************* TLS Server Interface *******************************
This file is part of the pdi stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 7th June 2026
******************************************************************************/
#include "TlsServerInterface.h"
#include "DeviceControlInterface.h"

#define TCP_GUARD_BEGIN \
    bool _pdi_need_lock = !sys_thread_tcpip(LWIP_CORE_LOCK_QUERY_HOLDER); \
    if (_pdi_need_lock) { LOCK_LWIP_TCPIP_CORE; }
#define TCP_GUARD_END \
    if (_pdi_need_lock) { UNLOCK_LWIP_TCPIP_CORE; }
// #define TCP_GUARD_BEGIN
// #define TCP_GUARD_END


TlsServerInterface::TlsServerInterface() :
    m_serverPcb(nullptr),
    m_clientPcb(nullptr),
    m_timeout(30000),
    m_pendingSince(0),
    m_hasClient(false),
    m_onAcceptCallbk(nullptr),
    m_onAcceptCallbkArg(nullptr) {}

TlsServerInterface::~TlsServerInterface() {
    close();
}

int32_t TlsServerInterface::begin(uint16_t port) {

    close();

    if (m_serverCertPath.empty() || m_serverKeyPath.empty()) {
        return -1;
    }

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

    m_serverPcb = tcp_listen_with_backlog(m_serverPcb, 1);
    if (!m_serverPcb) {
        TCP_GUARD_END
        return -99;
    }

    tcp_arg(m_serverPcb, this);
    tcp_accept(m_serverPcb, &TlsServerInterface::onAccept);
    m_hasClient = false;
    m_clientPcb = nullptr;
    TCP_GUARD_END
    return 0;
}

void TlsServerInterface::close() {
    TCP_GUARD_BEGIN
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
    TCP_GUARD_END
}

void TlsServerInterface::setOnAcceptClientEventCallback(CallBackVoidPointerArgFn callbk, void* arg) {
    m_onAcceptCallbk = callbk;
    m_onAcceptCallbkArg = arg;
}

bool TlsServerInterface::hasClient() const {
    TCP_GUARD_BEGIN
    bool v = m_hasClient && m_clientPcb;
    TCP_GUARD_END
    return v;
}

iClientInterface* TlsServerInterface::accept() {

    TCP_GUARD_BEGIN
    if (!(m_hasClient && m_clientPcb)) {
        TCP_GUARD_END
        return nullptr;
    }
    struct tcp_pcb* acceptedPcb = m_clientPcb;
    m_clientPcb    = nullptr;
    m_hasClient    = false;
    m_pendingSince = 0;

    tcp_arg (acceptedPcb, nullptr);
    tcp_recv(acceptedPcb, nullptr);
    tcp_err (acceptedPcb, nullptr);
    tcp_poll(acceptedPcb, nullptr, 0);
    TCP_GUARD_END

    TlsClientInterface* client = pdiutil::safe_new<TlsClientInterface>(acceptedPcb);
    if (!client) {
        LogE("TLS accept: OOM TlsClientInterface\n");
        TCP_GUARD_BEGIN
        tcp_abort(acceptedPcb);
        TCP_GUARD_END
        return nullptr;
    }

    const char* clientCa = m_clientCaPath.empty() ? nullptr : m_clientCaPath.c_str();
    if (!client->beginServer(m_serverCertPath.c_str(), m_serverKeyPath.c_str(), clientCa)) {
        pdiutil::safe_delete(client);
        return nullptr;
    }
    return client;
}

void TlsServerInterface::setTimeout(uint32_t timeout_ms) {
    m_timeout = timeout_ms;
}

bool TlsServerInterface::setServerCertificatePath(const char* path) {
    m_serverCertPath = path ? path : "";
    return !m_serverCertPath.empty();
}

bool TlsServerInterface::setServerPrivateKeyPath(const char* path) {
    m_serverKeyPath = path ? path : "";
    return !m_serverKeyPath.empty();
}

bool TlsServerInterface::setClientCertificateAuthorityPath(const char* path) {
    m_clientCaPath = path ? path : "";
    return !m_clientCaPath.empty();
}

void TlsServerInterface::onPendingError(void* arg, err_t /*err*/) {
    TlsServerInterface* server = static_cast<TlsServerInterface*>(arg);
    if (!server) return;
    server->m_clientPcb    = nullptr;
    server->m_hasClient    = false;
    server->m_pendingSince = 0;
}

err_t TlsServerInterface::onPendingRecv(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t /*err*/) {
    TlsServerInterface* server = static_cast<TlsServerInterface*>(arg);
    if (!server || !pcb) return ERR_ARG;

    if (p == nullptr) {
        if (server->m_clientPcb == pcb) {
            server->m_clientPcb    = nullptr;
            server->m_hasClient    = false;
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

err_t TlsServerInterface::onPendingPoll(void* arg, struct tcp_pcb* pcb) {
    TlsServerInterface* server = static_cast<TlsServerInterface*>(arg);
    if (!server || !pcb) return ERR_ABRT;

    if (server->m_clientPcb == pcb &&
        server->m_timeout > 0 &&
        (millis() - server->m_pendingSince) > server->m_timeout) {

        server->m_clientPcb    = nullptr;
        server->m_hasClient    = false;
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

err_t TlsServerInterface::onAccept(void* arg, struct tcp_pcb* newpcb, err_t /*err*/) {
    TlsServerInterface* server = static_cast<TlsServerInterface*>(arg);
    if (!server || !newpcb) return ERR_VAL;

    if (server->m_clientPcb) {
        struct tcp_pcb* old = server->m_clientPcb;
        server->m_clientPcb    = nullptr;
        server->m_hasClient    = false;
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
    tcp_err (newpcb, &TlsServerInterface::onPendingError);
    tcp_recv(newpcb, &TlsServerInterface::onPendingRecv);
    tcp_poll(newpcb, &TlsServerInterface::onPendingPoll, 4);

    if (server->m_onAcceptCallbk) {
        server->m_onAcceptCallbk(server->m_onAcceptCallbkArg);
    }

    return ERR_OK;
}
