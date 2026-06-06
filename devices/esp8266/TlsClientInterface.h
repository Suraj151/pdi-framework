/************************* TLS Client Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 2nd June 2026
******************************************************************************/
#ifndef _TLS_CLIENT_INTERFACE_H_
#define _TLS_CLIENT_INTERFACE_H_

#include "esp8266.h"
#include <interface/pdi/middlewares/iClientInterface.h>

#ifdef ENABLE_CONTEXTUAL_EXECUTION
#include "threading/PreemptiveMutex.h"
#endif

/**
 * @class TlsClientInterface
 * @brief A TLS client/server-side endpoint implementation that drives the
 *        BearSSL engine over the raw lwIP TCP API.
 *
 * One class serves both roles, selected at construction:
 *   - Server-accepted role  : constructed from a tcp_pcb* handed in by
 *                             TlsServerInterface::accept(). Drives the
 *                             BearSSL server engine after handshake.
 *   - Outbound client role  : default-constructed; user later calls
 *                             connect(host, port).
 *
 * BearSSL state is held behind a PIMPL-style nested struct so this header
 * does not need to expose <bearssl.h> to the wider framework.
 */
class TlsClientInterface : public iTlsClientInterface {
public:
    /**
     * @brief Constructor for outbound TLS client role.
     */
    TlsClientInterface();

    /**
     * @brief Constructor used when wrapping a server-accepted TCP pcb.
     * @param pcb Pointer to an existing TCP pcb just accepted by the listen socket.
     */
    TlsClientInterface(struct tcp_pcb* pcb);

    /**
     * @brief Destructor.
     */
    ~TlsClientInterface();

    /**
     * @brief Connect to a remote TLS server (outbound role only).
     * @param host The hostname or IP address of the server.
     * @param port The TCP port number.
     * @return Connection status (0 for success, negative for failure).
     */
    int16_t connect(const uint8_t *host, uint16_t port) override;

    /**
     * @brief Cleanly close the TLS connection (TLS close_notify, then TCP close).
     * @return 0 for success, negative for failure.
     */
    int16_t disconnect() override;

    /**
     * @brief Abort the connection without TLS close_notify.
     * @return 0 for success, negative for failure.
     */
    int16_t close() override;

    /**
     * @brief Check whether the TLS session is fully established.
     * @return 1 if the handshake has completed and the engine is open, 0 otherwise.
     */
    int8_t connected() override;

    /**
     * @brief Write plaintext data into the TLS engine.
     * @param c_str The data to send.
     * @param size The size of the data.
     * @return The number of bytes accepted by the engine, or 0/negative on failure.
     */
    int32_t write(const uint8_t* c_str, uint32_t size) override;

    int32_t write(uint8_t c) override { return write(&c, 1); }

    int32_t write(const uint8_t *c_str) override {
        return write(c_str, strlen(reinterpret_cast<const char*>(c_str)));
    }

    int32_t write_ro(const char *c_str) override;

    /**
     * @brief Read decrypted application data from the TLS engine.
     * @param buffer The buffer to fill.
     * @param size Maximum bytes to read.
     * @return Number of bytes read, or 0/negative on failure.
     */
    int32_t read(uint8_t* buffer, uint32_t size) override;

    uint8_t read() override { uint8_t byte = 0; read(&byte, 1); return byte; }

    /**
     * @brief Bytes of decrypted application data immediately available.
     * @return The number of bytes available, or -1 on error.
     */
    int32_t available() override;

    ipaddress_t getLocalIp() const override;
    uint16_t getLocalPort() const override;
    ipaddress_t getRemoteIp() const override;
    uint16_t getRemotePort() const override;

    bool setKeepAlive(uint16_t idleTime, uint16_t interval, uint16_t count) override;
    void setNoDelay(bool noDelay) override;
    void setTimeout(uint32_t timeout) override;
    bool availableforwrite(uint32_t size) override;
    void flush() override;

    /**
     * @brief Set the path to the trusted CA bundle for verifying the peer server.
     * @param path Filesystem path to a PEM/DER-encoded CA certificate.
     * @return True on success.
     */
    bool setCertificateAuthorityPath(const char* path) override;

    /**
     * @brief Set the path to the client certificate for mutual TLS.
     * @param path Filesystem path to a PEM/DER-encoded client certificate.
     * @return True on success.
     */
    bool setClientCertificatePath(const char* path) override;

    /**
     * @brief Set the path to the client private key for mutual TLS.
     * @param path Filesystem path to a PEM/DER-encoded private key.
     * @return True on success.
     */
    bool setClientPrivateKeyPath(const char* path) override;

    /**
     * @brief Set the SNI hostname presented during the TLS handshake.
     * @param hostname Null-terminated server name.
     */
    void setSNIHostname(const char* hostname) override;

    /**
     * @brief Toggle peer certificate verification. See iTlsClientInterface
     *        for the full semantics. Default is verify-on.
     */
    void setVerifyPeer(bool verify) override;

    /**
     * @brief Initialise BearSSL state for server-accepted role using the
     *        certificate/key paths provided by TlsServerInterface.
     *        Called by TlsServerInterface::accept() right after construction.
     * @param certPath     Filesystem path to the server certificate.
     * @param keyPath      Filesystem path to the server private key.
     * @param clientCaPath Optional path to a CA bundle used to verify client
     *                     certificates (mTLS). When non-null and non-empty,
     *                     the server will send a CertificateRequest and
     *                     validate the presented client cert.
     * @return True if BearSSL state was set up successfully.
     */
    bool beginServer(const char* certPath, const char* keyPath, const char* clientCaPath = nullptr);

private:
    enum Role : uint8_t {
        ROLE_CLIENT = 0,
        ROLE_SERVER = 1,
    };

    struct BearSSLState;

    struct dnsFoundResult {
        ip_addr_t addr;
        bool in_flight;
        bool found;
    };

    Role     m_role;
    struct tcp_pcb* m_pcb;
    bool     m_isConnected;
    uint32_t m_timeout;
    bool     m_isLastWriteAcked;

    BearSSLState* m_bear;

    uint8_t* m_rxQueue;
    uint32_t m_rxQueueLen;
    bool     m_pumpPending;
    volatile bool m_inPump;

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    int           m_taskId;
    volatile bool m_taskRunning;
    #endif

    pdiutil::string m_caPath;
    pdiutil::string m_certPath;
    pdiutil::string m_keyPath;
    pdiutil::string m_sniHostname;
    bool     m_verifyPeer;

    dnsFoundResult m_dns;

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    PreemptiveMutex m_mutex;
    #endif

    static err_t onConnected(void* arg, struct tcp_pcb* tpcb, err_t err);
    static err_t onReceive(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err);
    static void  onError(void* arg, err_t err);
    static err_t onSent(void* arg, struct tcp_pcb* tpcb, u16_t len);
    static void  onDnsFound(const char *name, const ip_addr_t *ipaddr, void *arg);

    void pumpEngine();
    void serviceRx();
    int16_t bearReset(int16_t rc);

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    bool startTlsWorker();
    void stopTlsWorker();
    #endif
};

#endif // _TLS_CLIENT_INTERFACE_H_
