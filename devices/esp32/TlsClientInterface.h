/************************* TLS Client Interface *******************************
This file is part of the pdi stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 7th June 2026
******************************************************************************/
#ifndef _TLS_CLIENT_INTERFACE_H_
#define _TLS_CLIENT_INTERFACE_H_

#include "esp32.h"
#include <interface/pdi/middlewares/iClientInterface.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class TlsClientInterface : public iTlsClientInterface {
public:
    TlsClientInterface();
    TlsClientInterface(struct tcp_pcb* pcb);
    ~TlsClientInterface();

    int16_t connect(const uint8_t *host, uint16_t port) override;
    int16_t disconnect() override;
    int16_t close() override;
    int8_t  connected() override;

    int32_t write(const uint8_t* c_str, uint32_t size) override;
    int32_t write(uint8_t c) override { return write(&c, 1); }
    int32_t write(const uint8_t *c_str) override {
        return write(c_str, strlen(reinterpret_cast<const char*>(c_str)));
    }
    int32_t write_ro(const char *c_str) override;

    int32_t read(uint8_t* buffer, uint32_t size) override;
    uint8_t read() override { uint8_t b = 0; read(&b, 1); return b; }

    int32_t  available() override;

    ipaddress_t getLocalIp() const override;
    uint16_t    getLocalPort() const override;
    ipaddress_t getRemoteIp() const override;
    uint16_t    getRemotePort() const override;

    bool setKeepAlive(uint16_t idleTime, uint16_t interval, uint16_t count) override;
    void setNoDelay(bool noDelay) override;
    void setTimeout(uint32_t timeout) override;
    bool availableforwrite(uint32_t size) override;
    void flush() override;

    bool setCertificateAuthorityPath(const char* path) override;
    bool setClientCertificatePath(const char* path) override;
    bool setClientPrivateKeyPath(const char* path) override;
    void setSNIHostname(const char* hostname) override;
    void setVerifyPeer(bool verify) override;

    bool beginServer(const char* certPath, const char* keyPath, const char* clientCaPath = nullptr);

private:
    enum Role : uint8_t {
        ROLE_CLIENT = 0,
        ROLE_SERVER = 1,
    };

    struct MbedTLSState;

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

    MbedTLSState* m_tls;

    uint8_t* m_rxQueue;
    uint32_t m_rxQueueLen;

    TaskHandle_t  m_workerHandle;
    volatile bool m_workerRunning;

    pdiutil::string m_caPath;
    pdiutil::string m_certPath;
    pdiutil::string m_keyPath;
    pdiutil::string m_sniHostname;
    bool     m_verifyPeer;

    dnsFoundResult m_dns;

    static err_t onConnected(void* arg, struct tcp_pcb* tpcb, err_t err);
    static err_t onReceive(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err);
    static void  onError(void* arg, err_t err);
    static err_t onSent(void* arg, struct tcp_pcb* tpcb, u16_t len);
    static void  onDnsFound(const char *name, const ip_addr_t *ipaddr, void *arg);

    static int bioSend(void* ctx, const unsigned char* buf, size_t len);
    static int bioRecv(void* ctx, unsigned char* buf, size_t len);

    static void workerThunk(void* arg);
    bool  startTlsWorker();
    void  stopTlsWorker();

    void  serviceTls();
    int16_t tlsReset(int16_t rc);
};

#endif
