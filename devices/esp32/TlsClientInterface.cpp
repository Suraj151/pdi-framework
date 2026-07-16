/************************* TLS Client Interface *******************************
This file is part of the pdi stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 7th June 2026
******************************************************************************/
#include "TlsClientInterface.h"
#include "DeviceControlInterface.h"
#include "MbedTLSCertLoader.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/net_sockets.h" // for MBEDTLS_ERR_NET_* codes
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include <stdlib.h>
#include <string.h>

#define TCP_GUARD_BEGIN \
    bool _pdi_need_lock = !sys_thread_tcpip(LWIP_CORE_LOCK_QUERY_HOLDER); \
    if (_pdi_need_lock) { LOCK_LWIP_TCPIP_CORE; }
#define TCP_GUARD_END \
    if (_pdi_need_lock) { UNLOCK_LWIP_TCPIP_CORE; }
// #define TCP_GUARD_BEGIN
// #define TCP_GUARD_END


struct TlsClientInterface::MbedTLSState {
    mbedtls_ssl_context       ssl;
    mbedtls_ssl_config        conf;
    mbedtls_x509_crt          caChain;
    mbedtls_x509_crt          ownCert;
    mbedtls_pk_context        ownKey;
    mbedtls_entropy_context   entropy;
    mbedtls_ctr_drbg_context  rng;

    bool sslInitDone;
    bool confInitDone;
    bool caLoaded;
    bool ownCertLoaded;
    bool ownKeyLoaded;
    bool handshakeDone;
    bool engineFatal;

    uint8_t peekByte;
    bool    peekValid;

    MbedTLSState();
    ~MbedTLSState();
};

TlsClientInterface::MbedTLSState::MbedTLSState() :
    sslInitDone(false),
    confInitDone(false),
    caLoaded(false),
    ownCertLoaded(false),
    ownKeyLoaded(false),
    handshakeDone(false),
    engineFatal(false),
    peekByte(0),
    peekValid(false)
{
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_x509_crt_init(&caChain);
    mbedtls_x509_crt_init(&ownCert);
    mbedtls_pk_init(&ownKey);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&rng);
    sslInitDone = true;
    confInitDone = true;
}

TlsClientInterface::MbedTLSState::~MbedTLSState() {
    if (sslInitDone)  mbedtls_ssl_free(&ssl);
    if (confInitDone) mbedtls_ssl_config_free(&conf);
    mbedtls_x509_crt_free(&caChain);
    mbedtls_x509_crt_free(&ownCert);
    mbedtls_pk_free(&ownKey);
    mbedtls_ctr_drbg_free(&rng);
    mbedtls_entropy_free(&entropy);
}


TlsClientInterface::TlsClientInterface() :
    m_role(ROLE_CLIENT),
    m_pcb(nullptr),
    m_isConnected(false),
    m_timeout(3000),
    m_isLastWriteAcked(true),
    m_tls(nullptr),
    m_rxQueue(nullptr),
    m_rxQueueLen(0),
    m_workerHandle(nullptr),
    m_workerRunning(false),
    m_verifyPeer(true),
    m_dns{IPADDR4_INIT(0), false, false}
{}

TlsClientInterface::TlsClientInterface(struct tcp_pcb* pcb) :
    m_role(ROLE_SERVER),
    m_pcb(pcb),
    m_isConnected(true),
    m_timeout(3000),
    m_isLastWriteAcked(true),
    m_tls(nullptr),
    m_rxQueue(nullptr),
    m_rxQueueLen(0),
    m_workerHandle(nullptr),
    m_workerRunning(false),
    m_verifyPeer(true),
    m_dns{IPADDR4_INIT(0), false, false}
{
    if (m_pcb) {
        TCP_GUARD_BEGIN
        tcp_arg(m_pcb, this);
        tcp_err(m_pcb, &TlsClientInterface::onError);
        tcp_recv(m_pcb, &TlsClientInterface::onReceive);
        tcp_sent(m_pcb, &TlsClientInterface::onSent);
        TCP_GUARD_END
        setNoDelay(true);
    }
}

int16_t TlsClientInterface::tlsReset(int16_t rc) {
    pdiutil::safe_delete(m_tls);
    return rc;
}

TlsClientInterface::~TlsClientInterface() {
    close();
    pdiutil::safe_delete(m_tls);
    pdiutil::safe_delete_array(m_rxQueue);
    m_rxQueueLen = 0;
}


bool TlsClientInterface::beginServer(const char* certPath, const char* keyPath, const char* clientCaPath) {
    if (m_role != ROLE_SERVER || !certPath || !keyPath) {
        LogE("TLS beginServer: bad args\n");
        return false;
    }

    pdiutil::safe_delete(m_tls);
    m_tls = pdiutil::safe_new<MbedTLSState>();
    if (!m_tls) {
        LogE("TLS beginServer: OOM\n");
        return false;
    }

    int rc = mbedtls_ctr_drbg_seed(&m_tls->rng, mbedtls_entropy_func, &m_tls->entropy, nullptr, 0);
    if (rc != 0) {
        LogFmtE("TLS beginServer: ctr_drbg_seed=%d\n", rc);
        tlsReset(0);
        return false;
    }

    if (!TlsCryptoLoader::loadCertChain(certPath, &m_tls->ownCert)) {
        LogFmtE("TLS beginServer: loadCertChain failed: %s\n", certPath);
        tlsReset(0);
        return false;
    }
    m_tls->ownCertLoaded = true;

    if (!TlsCryptoLoader::loadPrivateKey(keyPath, &m_tls->ownKey, &m_tls->rng)) {
        LogFmtE("TLS beginServer: loadPrivateKey failed: %s\n", keyPath);
        tlsReset(0);
        return false;
    }
    m_tls->ownKeyLoaded = true;

    rc = mbedtls_ssl_config_defaults(&m_tls->conf,
        MBEDTLS_SSL_IS_SERVER,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT);
    if (rc != 0) {
        LogFmtE("TLS beginServer: config_defaults=%d\n", rc);
        tlsReset(0);
        return false;
    }

    mbedtls_ssl_conf_rng(&m_tls->conf, mbedtls_ctr_drbg_random, &m_tls->rng);

    if (clientCaPath && clientCaPath[0]) {
        if (!TlsCryptoLoader::loadTrustAnchors(clientCaPath, &m_tls->caChain)) {
            LogFmtE("TLS beginServer: loadTrustAnchors failed: %s\n", clientCaPath);
            tlsReset(0);
            return false;
        }
        m_tls->caLoaded = true;
        mbedtls_ssl_conf_ca_chain(&m_tls->conf, &m_tls->caChain, nullptr);
        mbedtls_ssl_conf_authmode(&m_tls->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    } else {
        mbedtls_ssl_conf_authmode(&m_tls->conf, MBEDTLS_SSL_VERIFY_NONE);
    }

    rc = mbedtls_ssl_conf_own_cert(&m_tls->conf, &m_tls->ownCert, &m_tls->ownKey);
    if (rc != 0) {
        LogFmtE("TLS beginServer: conf_own_cert=%d\n", rc);
        tlsReset(0);
        return false;
    }

    rc = mbedtls_ssl_setup(&m_tls->ssl, &m_tls->conf);
    if (rc != 0) {
        LogFmtE("TLS beginServer: ssl_setup=%d\n", rc);
        tlsReset(0);
        return false;
    }

    mbedtls_ssl_set_bio(&m_tls->ssl, this,
        &TlsClientInterface::bioSend,
        &TlsClientInterface::bioRecv,
        nullptr);

    if (!startTlsWorker()) {
        LogE("TLS beginServer: startTlsWorker failed\n");
        tlsReset(0);
        return false;
    }

    LogI("TLS beginServer: ready\n");
    return true;
}


void TlsClientInterface::workerThunk(void* arg) {

    TlsClientInterface* self = static_cast<TlsClientInterface*>(arg);
    while (self && self->m_workerRunning) {
        self->serviceTls();
        vTaskDelay(pdMS_TO_TICKS(TLS_TASK_POLL_MS));
    }
    if (self) {
        self->m_workerHandle = nullptr;
    }
    vTaskDelete(NULL);
}

bool TlsClientInterface::startTlsWorker() {

    if (m_workerRunning) return true;

    m_workerRunning = true;
    BaseType_t ok = xTaskCreatePinnedToCore(
        &TlsClientInterface::workerThunk,
        "pdi_tls",
        TLS_TASK_STACK_SIZE / sizeof(StackType_t),
        this,
        tskIDLE_PRIORITY + 1,
        &m_workerHandle,
        tskNO_AFFINITY);

    if (ok != pdPASS) {
        m_workerRunning = false;
        m_workerHandle = nullptr;
        return false;
    }
    return true;
}

void TlsClientInterface::stopTlsWorker() {

    if (!m_workerRunning) return;
    m_workerRunning = false;

    uint32_t waited = 0;
    while (m_workerHandle != nullptr && waited < 200) {
        vTaskDelay(pdMS_TO_TICKS(5));
        waited += 5;
    }
    if (m_workerHandle != nullptr) {
        TaskHandle_t h = m_workerHandle;
        m_workerHandle = nullptr;
        vTaskDelete(h);
    }
}


bool TlsClientInterface::setCertificateAuthorityPath(const char* path) {
    m_caPath = path ? path : "";
    return !m_caPath.empty();
}

bool TlsClientInterface::setClientCertificatePath(const char* path) {
    m_certPath = path ? path : "";
    return !m_certPath.empty();
}

bool TlsClientInterface::setClientPrivateKeyPath(const char* path) {
    m_keyPath = path ? path : "";
    return !m_keyPath.empty();
}

void TlsClientInterface::setSNIHostname(const char* hostname) {
    m_sniHostname = hostname ? hostname : "";
}

void TlsClientInterface::setVerifyPeer(bool verify) {
    m_verifyPeer = verify;
}

void TlsClientInterface::setTimeout(uint32_t timeout) {
    m_timeout = timeout;
}


int16_t TlsClientInterface::connect(const uint8_t* host, uint16_t port) {
    if (m_role != ROLE_CLIENT || !host) return -1;
    if (m_verifyPeer && m_caPath.empty()) {
        LogE("TLS connect: verify-peer is on but no CA path set\n");
        return -2;
    }

    close();

    uint32_t start = __i_dvc_ctrl.millis_now();
    const char* hostname = reinterpret_cast<const char*>(host);
    ip_addr_t serverIp;

    if (!ipaddr_aton(hostname, &serverIp)) {
        struct addrinfo *res = nullptr;
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;

        int gerr = lwip_getaddrinfo(hostname, "0", &hints, &res);
        if (gerr == 0 && res) {
            if (res->ai_family == AF_INET6) {
                lwip_freeaddrinfo(res);
                return -100;
            }
            serverIp.type = IPADDR_TYPE_V4;
            serverIp.u_addr.ip4.addr = ((struct sockaddr_in*)res->ai_addr)->sin_addr.s_addr;
            lwip_freeaddrinfo(res);
        } else {
            if (res) lwip_freeaddrinfo(res);
            LogFmtE("TLS connect: DNS resolution failed for %s\n", hostname);
            return -100;
        }
    }

    pdiutil::safe_delete(m_tls);
    m_tls = pdiutil::safe_new<MbedTLSState>();
    if (!m_tls) {
        LogE("TLS connect: OOM\n");
        return -97;
    }

    int rc = mbedtls_ctr_drbg_seed(&m_tls->rng, mbedtls_entropy_func, &m_tls->entropy, nullptr, 0);
    if (rc != 0) {
        LogFmtE("TLS connect: ctr_drbg_seed=%d\n", rc);
        return tlsReset(-97);
    }

    if (m_verifyPeer && !m_caPath.empty()) {
        if (!TlsCryptoLoader::loadTrustAnchors(m_caPath.c_str(), &m_tls->caChain)) {
            LogFmtE("TLS connect: loadTrustAnchors failed: %s\n", m_caPath.c_str());
            return tlsReset(-3);
        }
        m_tls->caLoaded = true;
    }

    rc = mbedtls_ssl_config_defaults(&m_tls->conf,
        MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT);
    if (rc != 0) {
        LogFmtE("TLS connect: config_defaults=%d\n", rc);
        return tlsReset(-5);
    }

    mbedtls_ssl_conf_rng(&m_tls->conf, mbedtls_ctr_drbg_random, &m_tls->rng);

    if (m_verifyPeer) {
        mbedtls_ssl_conf_authmode(&m_tls->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
        if (m_tls->caLoaded) {
            mbedtls_ssl_conf_ca_chain(&m_tls->conf, &m_tls->caChain, nullptr);
        }
    } else {
        mbedtls_ssl_conf_authmode(&m_tls->conf, MBEDTLS_SSL_VERIFY_NONE);
    }

    if (!m_certPath.empty() && !m_keyPath.empty()) {
        if (!TlsCryptoLoader::loadCertChain(m_certPath.c_str(), &m_tls->ownCert)) {
            LogFmtE("TLS connect: loadCertChain failed: %s\n", m_certPath.c_str());
            return tlsReset(-4);
        }
        m_tls->ownCertLoaded = true;
        if (!TlsCryptoLoader::loadPrivateKey(m_keyPath.c_str(), &m_tls->ownKey, &m_tls->rng)) {
            LogFmtE("TLS connect: loadPrivateKey failed: %s\n", m_keyPath.c_str());
            return tlsReset(-4);
        }
        m_tls->ownKeyLoaded = true;
        rc = mbedtls_ssl_conf_own_cert(&m_tls->conf, &m_tls->ownCert, &m_tls->ownKey);
        if (rc != 0) {
            LogFmtE("TLS connect: conf_own_cert=%d\n", rc);
            return tlsReset(-4);
        }
    }

    rc = mbedtls_ssl_setup(&m_tls->ssl, &m_tls->conf);
    if (rc != 0) {
        LogFmtE("TLS connect: ssl_setup=%d\n", rc);
        return tlsReset(-5);
    }

    const char* sni = m_sniHostname.empty() ? hostname : m_sniHostname.c_str();
    rc = mbedtls_ssl_set_hostname(&m_tls->ssl, sni);
    if (rc != 0) {
        LogFmtE("TLS connect: set_hostname=%d\n", rc);
        return tlsReset(-5);
    }

    mbedtls_ssl_set_bio(&m_tls->ssl, this,
        &TlsClientInterface::bioSend,
        &TlsClientInterface::bioRecv,
        nullptr);

    TCP_GUARD_BEGIN
    m_pcb = tcp_new();
    if (!m_pcb) {
        TCP_GUARD_END
        return tlsReset(-98);
    }
    tcp_arg(m_pcb, this);
    tcp_err(m_pcb, &TlsClientInterface::onError);
    tcp_recv(m_pcb, &TlsClientInterface::onReceive);
    tcp_sent(m_pcb, &TlsClientInterface::onSent);
    err_t terr = tcp_connect(m_pcb, &serverIp, port, &TlsClientInterface::onConnected);
    TCP_GUARD_END
    if (terr != ERR_OK) {
        close();
        return terr < 0 ? terr : -97;
    }
    setNoDelay(true);

    if (!startTlsWorker()) {
        close();
        return -97;
    }

    while (!connected() && (__i_dvc_ctrl.millis_now() - start) < m_timeout) {
        __i_dvc_ctrl.wait(1);
    }

    if (!connected()) {
        uint32_t elapsed = __i_dvc_ctrl.millis_now() - start;
        LogFmtE("TLS connect: timeout after %u ms (tcp.connected=%d rxQ=%u)\n",
            (unsigned)elapsed, (int)m_isConnected, (unsigned)m_rxQueueLen);
        close();
        return -101;
    }

    return 0;
}

int16_t TlsClientInterface::disconnect() {
    return close();
}

int16_t TlsClientInterface::close() {

    stopTlsWorker();

    if (m_tls && m_tls->handshakeDone && !m_tls->engineFatal) {
        mbedtls_ssl_close_notify(&m_tls->ssl);
    }

    if (m_rxQueue) {
        TCP_GUARD_BEGIN
        if (m_pcb && m_rxQueueLen > 0) {
            tcp_recved(m_pcb, m_rxQueueLen);
        }
        TCP_GUARD_END
        pdiutil::safe_delete_array(m_rxQueue);
        m_rxQueueLen = 0;
    }

    TCP_GUARD_BEGIN
    struct tcp_pcb* _p = m_pcb;
    m_pcb = nullptr;
    if (_p) {
        tcp_arg(_p, nullptr);
        tcp_err(_p, nullptr);
        tcp_recv(_p, nullptr);
        tcp_sent(_p, nullptr);
        tcp_close(_p);
    }
    TCP_GUARD_END

    m_isConnected = false;

    pdiutil::safe_delete(m_tls);
    return 0;
}

int8_t TlsClientInterface::connected() {
    if (!m_isConnected || !m_tls || m_tls->engineFatal) return 0;
    if (m_role == ROLE_SERVER) return 1;
    return m_tls->handshakeDone ? 1 : 0;
}


int TlsClientInterface::bioSend(void* ctx, const unsigned char* buf, size_t len) {

    TlsClientInterface* self = static_cast<TlsClientInterface*>(ctx);
    if (!self || !self->m_pcb || !self->m_isConnected) return MBEDTLS_ERR_SSL_CONN_EOF;

    TCP_GUARD_BEGIN
    uint16_t sndbuf = tcp_sndbuf(self->m_pcb);
    if (sndbuf == 0) {
        TCP_GUARD_END
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    }

    uint16_t toSend = (len < sndbuf) ? (uint16_t)len : sndbuf;
    uint8_t flags = TCP_WRITE_FLAG_COPY;
    if (!self->m_isLastWriteAcked) flags |= TCP_WRITE_FLAG_MORE;

    err_t err = tcp_write(self->m_pcb, buf, toSend, flags);
    if (err == ERR_OK) {
        if (self->m_isLastWriteAcked) {
            self->m_isLastWriteAcked = false;
            tcp_output(self->m_pcb);
        }
        TCP_GUARD_END
        return (int)toSend;
    }
    TCP_GUARD_END

    if (err == ERR_MEM) return MBEDTLS_ERR_SSL_WANT_WRITE;
    return MBEDTLS_ERR_NET_SEND_FAILED;
}

int TlsClientInterface::bioRecv(void* ctx, unsigned char* buf, size_t len) {

    TlsClientInterface* self = static_cast<TlsClientInterface*>(ctx);
    if (!self) return MBEDTLS_ERR_SSL_CONN_EOF;

    TCP_GUARD_BEGIN
    if (self->m_rxQueueLen == 0) {
        TCP_GUARD_END
        if (!self->m_isConnected) return MBEDTLS_ERR_SSL_CONN_EOF;
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    uint32_t chunk = (self->m_rxQueueLen < len) ? self->m_rxQueueLen : (uint32_t)len;
    memcpy(buf, self->m_rxQueue, chunk);

    if (chunk < self->m_rxQueueLen) {
        memmove(self->m_rxQueue, self->m_rxQueue + chunk, self->m_rxQueueLen - chunk);
        self->m_rxQueueLen -= chunk;
    } else {
        pdiutil::safe_delete_array(self->m_rxQueue);
        self->m_rxQueueLen = 0;
    }

    if (self->m_pcb) {
        tcp_recved(self->m_pcb, chunk);
    }
    TCP_GUARD_END
    return (int)chunk;
}


int32_t TlsClientInterface::write(const uint8_t* data, uint32_t size) {

    if (!data || size == 0) return 0;
    if (!m_tls || !m_pcb || m_tls->engineFatal) return 0;
    if (!m_tls->handshakeDone) return 0;

    uint32_t written = 0;
    while (written < size) {
        int rc = mbedtls_ssl_write(&m_tls->ssl, data + written, size - written);
        if (rc > 0) {
            written += rc;
            continue;
        }
        if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
            __i_dvc_ctrl.wait(1);
            continue;
        }
        LogFmtE("TLS write: ssl_write=%d\n", rc);
        m_tls->engineFatal = true;
        break;
    }
    return (int32_t)written;
}

int32_t TlsClientInterface::write_ro(const char* c_str) {
    return c_str ? write(reinterpret_cast<const uint8_t*>(c_str), strlen(c_str)) : 0;
}

int32_t TlsClientInterface::read(uint8_t* buffer, uint32_t size) {

    if (!buffer || size == 0) return 0;
    if (!m_tls || m_tls->engineFatal) return 0;
    if (!m_tls->handshakeDone) return 0;

    uint32_t total = 0;
    if (m_tls->peekValid) {
        buffer[total++] = m_tls->peekByte;
        m_tls->peekValid = false;
    }
    if (total >= size) return (int32_t)total;

    int rc = mbedtls_ssl_read(&m_tls->ssl, buffer + total, size - total);
    if (rc > 0) return (int32_t)(total + rc);
    if (rc == 0) return (int32_t)total;
    if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) return (int32_t)total;
    if (rc == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
        m_isConnected = false;
        return (int32_t)total;
    }
    LogFmtE("TLS read: ssl_read=%d\n", rc);
    m_tls->engineFatal = true;
    m_isConnected = false;
    return (int32_t)total;
}

int32_t TlsClientInterface::available() {

    if (!m_tls || m_tls->engineFatal || !m_tls->handshakeDone) return 0;

    if (m_tls->peekValid) {
        return (int32_t)(1 + mbedtls_ssl_get_bytes_avail(&m_tls->ssl));
    }

    size_t avail = mbedtls_ssl_get_bytes_avail(&m_tls->ssl);
    if (avail > 0) return (int32_t)avail;

    if (m_rxQueueLen == 0) return 0;

    uint8_t p = 0;
    int rc = mbedtls_ssl_read(&m_tls->ssl, &p, 1);
    if (rc == 1) {
        m_tls->peekByte  = p;
        m_tls->peekValid = true;
        return (int32_t)(1 + mbedtls_ssl_get_bytes_avail(&m_tls->ssl));
    }
    if (rc == 0 || rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
        return 0;
    }
    if (rc == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
        m_isConnected = false;
        return 0;
    }
    LogFmtE("TLS available: ssl_read=%d\n", rc);
    m_tls->engineFatal = true;
    m_isConnected = false;
    return 0;
}

ipaddress_t TlsClientInterface::getLocalIp() const {
    TCP_GUARD_BEGIN
    ipaddress_t v = m_pcb ? m_pcb->local_ip.u_addr.ip4.addr : 0;
    TCP_GUARD_END
    return v;
}
uint16_t TlsClientInterface::getLocalPort() const {
    TCP_GUARD_BEGIN
    uint16_t v = m_pcb ? m_pcb->local_port : 0;
    TCP_GUARD_END
    return v;
}
ipaddress_t TlsClientInterface::getRemoteIp() const {
    TCP_GUARD_BEGIN
    ipaddress_t v = m_pcb ? m_pcb->remote_ip.u_addr.ip4.addr : 0;
    TCP_GUARD_END
    return v;
}
uint16_t TlsClientInterface::getRemotePort() const {
    TCP_GUARD_BEGIN
    uint16_t v = m_pcb ? m_pcb->remote_port : 0;
    TCP_GUARD_END
    return v;
}

bool TlsClientInterface::setKeepAlive(uint16_t, uint16_t, uint16_t) {
    return false;
}

void TlsClientInterface::setNoDelay(bool noDelay) {
    TCP_GUARD_BEGIN
    if (m_pcb) {
        if (noDelay) m_pcb->so_options |= TF_NODELAY;
        else         m_pcb->so_options &= ~TF_NODELAY;
    }
    TCP_GUARD_END
}

bool TlsClientInterface::availableforwrite(uint32_t size) {

    __i_dvc_ctrl.yield();
    if (!m_pcb || !m_isConnected) return false;

    TCP_GUARD_BEGIN
    if (m_pcb->state != ESTABLISHED &&
        m_pcb->state != CLOSE_WAIT &&
        m_pcb->state != SYN_SENT &&
        m_pcb->state != SYN_RCVD) {
        TCP_GUARD_END
        return false;
    }
    tcp_output(m_pcb);
    uint32_t availablebuff = tcp_sndbuf(m_pcb);
    uint32_t queuelen = tcp_sndqueuelen(m_pcb);
    TCP_GUARD_END

    if ((availablebuff < size) ||
        (queuelen >= TCP_SND_QUEUELEN) ||
        (queuelen > TCP_SNDQUEUELEN_OVERFLOW)) {
        __i_dvc_ctrl.yield();
        return false;
    }
    __i_dvc_ctrl.yield();
    return m_isLastWriteAcked;
}

void TlsClientInterface::flush() {
    TCP_GUARD_BEGIN
    if (m_pcb) tcp_output(m_pcb);
    TCP_GUARD_END
    m_isLastWriteAcked = true;
}


err_t TlsClientInterface::onConnected(void* arg, struct tcp_pcb*, err_t err) {
    TlsClientInterface* self = static_cast<TlsClientInterface*>(arg);
    if (!self) return ERR_VAL;
    if (err != ERR_OK) {
        if (self->m_tls) self->m_tls->engineFatal = true;
        return err;
    }
    self->m_isConnected = true;
    return ERR_OK;
}

err_t TlsClientInterface::onReceive(void* arg, struct tcp_pcb*, struct pbuf* p, err_t err) {
    TlsClientInterface* self = static_cast<TlsClientInterface*>(arg);
    if (!self) {
        if (p) pbuf_free(p);
        return ERR_VAL;
    }
    if (err != ERR_OK || !p) {
        if (p) pbuf_free(p);
        if (err != ERR_OK && self->m_tls) self->m_tls->engineFatal = true;
        return ERR_OK;
    }

    uint32_t newSize = self->m_rxQueueLen + p->tot_len;
    if((newSize+self->m_rxQueueLen) > (0.90*TLS_IBUF_SIZE)){
        return ERR_MEM;
    }

    uint8_t* newBuf = pdiutil::safe_new_array<uint8_t>(newSize);
    if (!newBuf) {
        LogFmtE("TLS onReceive: alloc fail, in=%u rxQ=%u\n",
            (unsigned)p->tot_len, (unsigned)self->m_rxQueueLen);
        return ERR_MEM;
    }
    if (self->m_rxQueue) {
        memcpy(newBuf, self->m_rxQueue, self->m_rxQueueLen);
        pdiutil::safe_delete_array(self->m_rxQueue);
    }
    pbuf_copy_partial(p, newBuf + self->m_rxQueueLen, p->tot_len, 0);
    self->m_rxQueue = newBuf;
    self->m_rxQueueLen = newSize;

    pbuf_free(p);
    return ERR_OK;
}

void TlsClientInterface::onError(void* arg, err_t /*err*/) {
    TlsClientInterface* self = static_cast<TlsClientInterface*>(arg);
    if (!self) return;
    self->m_pcb = nullptr;
    self->m_isConnected = false;
    if (self->m_tls) self->m_tls->engineFatal = true;
}

err_t TlsClientInterface::onSent(void* arg, struct tcp_pcb*, u16_t) {
    TlsClientInterface* self = static_cast<TlsClientInterface*>(arg);
    if (!self) return ERR_VAL;
    self->m_isLastWriteAcked = true;
    return ERR_OK;
}

void TlsClientInterface::onDnsFound(const char*, const ip_addr_t* ipaddr, void* arg) {
    TlsClientInterface* self = static_cast<TlsClientInterface*>(arg);
    if (!self) return;
    if (ipaddr) {
        self->m_dns.addr = *ipaddr;
        self->m_dns.found = true;
    } else {
        self->m_dns.found = false;
    }
    self->m_dns.in_flight = false;
}


void TlsClientInterface::serviceTls() {

    if (!m_tls || m_tls->engineFatal) return;
    if (!m_pcb || !m_isConnected) return;

    if (!m_tls->handshakeDone) {
        int rc = mbedtls_ssl_handshake(&m_tls->ssl);
        if (rc == 0) {
            m_tls->handshakeDone = true;
        } else if (rc != MBEDTLS_ERR_SSL_WANT_READ && rc != MBEDTLS_ERR_SSL_WANT_WRITE) {
            LogFmtE("TLS serviceTls: handshake=%d\n", rc);
            m_tls->engineFatal = true;
        }
    }
}
