/************************* TLS Server Interface *******************************
This file is part of the pdi stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 7th June 2026
******************************************************************************/
#ifndef _TLS_SERVER_INTERFACE_H_
#define _TLS_SERVER_INTERFACE_H_

#include "esp32.h"
#include "TlsClientInterface.h"
#include <interface/pdi/middlewares/iServerInterface.h>

class TlsServerInterface : public iTlsServerInterface {
public:
    TlsServerInterface();
    ~TlsServerInterface();

    int32_t begin(uint16_t port) override;
    bool    hasClient() const override;
    iClientInterface* accept() override;
    void setTimeout(uint32_t timeout_ms) override;
    void close() override;
    void setOnAcceptClientEventCallback(CallBackVoidPointerArgFn callbk, void* arg = nullptr) override;

    bool setServerCertificatePath(const char* path) override;
    bool setServerPrivateKeyPath(const char* path) override;
    bool setClientCertificateAuthorityPath(const char* path) override;

private:
    static err_t onAccept(void* arg, struct tcp_pcb* newpcb, err_t err);
    static void  onPendingError(void* arg, err_t err);

    struct tcp_pcb* m_serverPcb;
    struct tcp_pcb* m_clientPcb;
    uint32_t m_timeout;
    bool     m_hasClient;

    pdiutil::string m_serverCertPath;
    pdiutil::string m_serverKeyPath;
    pdiutil::string m_clientCaPath;

    CallBackVoidPointerArgFn m_onAcceptCallbk;
    void* m_onAcceptCallbkArg;
};

#endif
