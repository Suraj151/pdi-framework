/************************* TLS Server Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 2nd June 2026
******************************************************************************/
#ifndef _TLS_SERVER_INTERFACE_H_
#define _TLS_SERVER_INTERFACE_H_

#include "esp8266.h"
#include "TlsClientInterface.h"
#include <interface/pdi/middlewares/iServerInterface.h>

/**
 * @class TlsServerInterface
 * @brief TLS-secured TCP listening server using the raw lwIP API for accept,
 *        with BearSSL driven inside the per-client TlsClientInterface.
 *
 * Certificate and private-key paths must be configured before begin(port);
 * begin() loads the PEM/DER material from the filesystem so the same
 * cert/key can be reused for every accepted client without reloading.
 */
class TlsServerInterface : public iTlsServerInterface {
public:
    TlsServerInterface();
    ~TlsServerInterface();

    /**
     * @brief Begin listening on the specified TCP port.
     * @param port The port number to listen on.
     * @return 0 on success, negative error code on failure.
     */
    int32_t begin(uint16_t port) override;

    /**
     * @brief Check if there is a client connected and waiting to be accepted.
     * @return True if a client is connected, false otherwise.
     */
    bool hasClient() const override;

    /**
     * @brief Accept the pending client, wrap it in a TLS endpoint, and return it.
     *        The handshake is driven inside the returned TlsClientInterface as
     *        data arrives through lwIP callbacks.
     * @return Pointer to the accepted client interface, or nullptr if none.
     */
    iClientInterface* accept() override;

    /**
     * @brief Set the timeout for client connections.
     * @param timeout_ms The timeout value in milliseconds.
     */
    void setTimeout(uint32_t timeout_ms) override;

    /**
     * @brief Close the server and any pending client pcb.
     */
    void close() override;

    /**
     * @brief Set callback invoked when a new client connection is accepted by lwIP.
     */
    void setOnAcceptClientEventCallback(CallBackVoidPointerArgFn callbk, void* arg = nullptr) override;

    /**
     * @brief Set the path to the server certificate.
     */
    bool setServerCertificatePath(const char* path) override;

    /**
     * @brief Set the path to the server private key.
     */
    bool setServerPrivateKeyPath(const char* path) override;

    /**
     * @brief Set the path to the CA used to verify client certificates (mutual TLS).
     */
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

#endif // _TLS_SERVER_INTERFACE_H_
