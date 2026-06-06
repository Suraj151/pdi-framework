/************************* TLS Client Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 2nd June 2026
******************************************************************************/
#include "TlsClientInterface.h"
#include "DeviceControlInterface.h"
#include "BearSSLCertLoader.h"
#include <bearssl/bearssl.h>
#include <stdlib.h>

#ifndef ENABLE_CONTEXTUAL_EXECUTION
#error "ENABLE_CONTEXTUAL_EXECUTION is required for TLS on ESP8266: BearSSL handshake cannot fit in the default cont_t stack. Define ENABLE_CONTEXTUAL_EXECUTION in your sketch / build flags."
#endif

#define TLS_ENG ((m_role == ROLE_SERVER) ? &m_bear->sc.eng : &m_bear->cc.eng)

static void tls_client_engine_base_init(br_ssl_client_context* cc,
                                        const uint16_t* suites,
                                        size_t suiteCount) {
    br_ssl_client_zero(cc);
    br_ssl_engine_add_flags(&cc->eng, BR_OPT_NO_RENEGOTIATION);
    br_ssl_engine_set_versions(&cc->eng, BR_TLS12, BR_TLS12);
    br_ssl_engine_set_suites(&cc->eng, suites, suiteCount);
    br_ssl_client_set_default_rsapub(cc);
    br_ssl_engine_set_default_rsavrfy(&cc->eng);
    br_ssl_engine_set_default_ecdsa(&cc->eng);
    br_ssl_engine_set_default_ec(&cc->eng);
    br_ssl_engine_set_hash(&cc->eng, br_md5_ID, &br_md5_vtable);
    br_ssl_engine_set_hash(&cc->eng, br_sha1_ID, &br_sha1_vtable);
    br_ssl_engine_set_hash(&cc->eng, br_sha224_ID, &br_sha224_vtable);
    br_ssl_engine_set_hash(&cc->eng, br_sha256_ID, &br_sha256_vtable);
    br_ssl_engine_set_hash(&cc->eng, br_sha384_ID, &br_sha384_vtable);
    br_ssl_engine_set_hash(&cc->eng, br_sha512_ID, &br_sha512_vtable);
    br_ssl_engine_set_prf_sha256(&cc->eng, &br_tls12_sha256_prf);
    br_ssl_engine_set_default_aes_gcm(&cc->eng);
}

static void tls_x509_minimal_install(br_x509_minimal_context* xc,
                                     const br_x509_trust_anchor* anchors,
                                     size_t anchorCount) {
    br_x509_minimal_init(xc, &br_sha256_vtable, anchors, anchorCount);
    br_x509_minimal_set_hash(xc, br_md5_ID, &br_md5_vtable);
    br_x509_minimal_set_hash(xc, br_sha1_ID, &br_sha1_vtable);
    br_x509_minimal_set_hash(xc, br_sha224_ID, &br_sha224_vtable);
    br_x509_minimal_set_hash(xc, br_sha256_ID, &br_sha256_vtable);
    br_x509_minimal_set_hash(xc, br_sha384_ID, &br_sha384_vtable);
    br_x509_minimal_set_hash(xc, br_sha512_ID, &br_sha512_vtable);
    br_x509_minimal_set_rsa(xc, br_rsa_pkcs1_vrfy_get_default());
    br_x509_minimal_set_ecdsa(xc, br_ec_get_default(), br_ecdsa_vrfy_asn1_get_default());
}

struct NoopX509State {
    const br_x509_class*       vtable;
    br_x509_decoder_context    decoder;
    bool                       is_leaf;
    bool                       have_pkey;
};

static void noop_x509_start_chain(const br_x509_class** ctx, const char* server_name) {
    NoopX509State* s = reinterpret_cast<NoopX509State*>(ctx);
    s->is_leaf = true;
    s->have_pkey = false;
    (void)server_name;
}

static void noop_x509_start_cert(const br_x509_class** ctx, uint32_t length) {
    NoopX509State* s = reinterpret_cast<NoopX509State*>(ctx);
    (void)length;
    if (s->is_leaf) {
        br_x509_decoder_init(&s->decoder, nullptr, nullptr, nullptr, nullptr);
    }
}

static void noop_x509_append(const br_x509_class** ctx, const unsigned char* buf, size_t len) {
    NoopX509State* s = reinterpret_cast<NoopX509State*>(ctx);
    if (s->is_leaf) {
        br_x509_decoder_push(&s->decoder, buf, len);
    }
}

static void noop_x509_end_cert(const br_x509_class** ctx) {
    NoopX509State* s = reinterpret_cast<NoopX509State*>(ctx);
    if (s->is_leaf) {
        if (br_x509_decoder_last_error(&s->decoder) == 0) {
            s->have_pkey = true;
        }
        s->is_leaf = false;
    }
}

static unsigned noop_x509_end_chain(const br_x509_class** ctx) {
    NoopX509State* s = reinterpret_cast<NoopX509State*>(ctx);
    return s->have_pkey ? 0u : (unsigned)BR_ERR_X509_NOT_TRUSTED;
}

static const br_x509_pkey* noop_x509_get_pkey(const br_x509_class* const* ctx, unsigned* usages) {
    const NoopX509State* s = reinterpret_cast<const NoopX509State*>(ctx);
    if (usages) {
        *usages = BR_KEYTYPE_KEYX | BR_KEYTYPE_SIGN;
    }
    return s->have_pkey ? br_x509_decoder_get_pkey(const_cast<br_x509_decoder_context*>(&s->decoder)) : nullptr;
}

static const br_x509_class noop_x509_vtable = {
    sizeof(NoopX509State),
    noop_x509_start_chain,
    noop_x509_start_cert,
    noop_x509_append,
    noop_x509_end_cert,
    noop_x509_end_chain,
    noop_x509_get_pkey,
};

static void tls_server_engine_base_init(br_ssl_server_context* sc,
                                        const uint16_t* suites,
                                        size_t suiteCount) {
    br_ssl_server_zero(sc);
    br_ssl_engine_add_flags(&sc->eng, BR_OPT_NO_RENEGOTIATION);
    br_ssl_engine_set_versions(&sc->eng, BR_TLS12, BR_TLS12);
    br_ssl_engine_set_suites(&sc->eng, suites, suiteCount);
    br_ssl_engine_set_default_ec(&sc->eng);
    br_ssl_engine_set_hash(&sc->eng, br_md5_ID, &br_md5_vtable);
    br_ssl_engine_set_hash(&sc->eng, br_sha1_ID, &br_sha1_vtable);
    br_ssl_engine_set_hash(&sc->eng, br_sha224_ID, &br_sha224_vtable);
    br_ssl_engine_set_hash(&sc->eng, br_sha256_ID, &br_sha256_vtable);
    br_ssl_engine_set_hash(&sc->eng, br_sha384_ID, &br_sha384_vtable);
    br_ssl_engine_set_hash(&sc->eng, br_sha512_ID, &br_sha512_vtable);
    br_ssl_engine_set_prf_sha256(&sc->eng, &br_tls12_sha256_prf);
    br_ssl_engine_set_default_aes_gcm(&sc->eng);
}


struct TlsClientInterface::BearSSLState {
    union {
        br_ssl_server_context sc;
        br_ssl_client_context cc;
    };
    br_x509_minimal_context xc;

    br_x509_certificate* certChain;
    size_t               certChainCount;
    uint8_t*             certBacking;
    br_skey_decoder_context skey;

    br_x509_trust_anchor* trustAnchors;
    size_t                trustAnchorsCount;

    uint8_t* ibuf;
    size_t   ibufLen;
    uint8_t* obuf;
    size_t   obufLen;

    NoopX509State* noopX509;

    bool handshakeDone;
    bool engineFatal;

    BearSSLState();
    ~BearSSLState();

    bool allocIoBuffers(size_t ibufSize, size_t obufSize);
};

TlsClientInterface::BearSSLState::BearSSLState() :
    certChain(nullptr),
    certChainCount(0),
    certBacking(nullptr),
    trustAnchors(nullptr),
    trustAnchorsCount(0),
    ibuf(nullptr),
    ibufLen(0),
    obuf(nullptr),
    obufLen(0),
    noopX509(nullptr),
    handshakeDone(false),
    engineFatal(false)
{
    memset(&sc, 0, sizeof(sc));
    memset(&cc, 0, sizeof(cc));
    memset(&xc, 0, sizeof(xc));
    memset(&skey, 0, sizeof(skey));
}

TlsClientInterface::BearSSLState::~BearSSLState() {
    TlsCryptoLoader::freeCertChain(certChain, certChainCount, certBacking);
    TlsCryptoLoader::freeTrustAnchors(trustAnchors, trustAnchorsCount);
    if (ibuf != nullptr) {
        free(ibuf);
        ibuf = nullptr;
    }
    if (obuf != nullptr) {
        free(obuf);
        obuf = nullptr;
    }
    pdiutil::safe_delete(noopX509);
}

bool TlsClientInterface::BearSSLState::allocIoBuffers(size_t ibufSize, size_t obufSize) {
    ibuf = static_cast<uint8_t*>(malloc(ibufSize));
    obuf = static_cast<uint8_t*>(malloc(obufSize));
    if (!ibuf || !obuf) {
        if (ibuf != nullptr) {
            free(ibuf);
            ibuf = nullptr;
        }
        if (obuf != nullptr) {
            free(obuf);
            obuf = nullptr;
        }
        return false;
    }
    ibufLen = ibufSize;
    obufLen = obufSize;
    return true;
}


TlsClientInterface::TlsClientInterface() :
    m_role(ROLE_CLIENT),
    m_pcb(nullptr),
    m_isConnected(false),
    m_timeout(3000),
    m_isLastWriteAcked(true),
    m_bear(nullptr),
    m_rxQueue(nullptr),
    m_rxQueueLen(0),
    m_pumpPending(false),
    m_inPump(false),
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_taskId(-1),
    m_taskRunning(false),
    #endif
    m_verifyPeer(true),
    m_dns{IP4_ADDRESS_NONE, false, false}
{}

TlsClientInterface::TlsClientInterface(struct tcp_pcb* pcb) :
    m_role(ROLE_SERVER),
    m_pcb(pcb),
    m_isConnected(true),
    m_timeout(3000),
    m_isLastWriteAcked(true),
    m_bear(nullptr),
    m_rxQueue(nullptr),
    m_rxQueueLen(0),
    m_pumpPending(false),
    m_inPump(false),
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_taskId(-1),
    m_taskRunning(false),
    #endif
    m_verifyPeer(true),
    m_dns{IP4_ADDRESS_NONE, false, false}
{
    if (m_pcb) {
        tcp_arg(m_pcb, this);
        tcp_err(m_pcb, &TlsClientInterface::onError);
        tcp_recv(m_pcb, &TlsClientInterface::onReceive);
        tcp_sent(m_pcb, &TlsClientInterface::onSent);
        setNoDelay(true);
    }
}

int16_t TlsClientInterface::bearReset(int16_t rc) {
    pdiutil::safe_delete(m_bear);
    return rc;
}

TlsClientInterface::~TlsClientInterface() {
    close();
    pdiutil::safe_delete(m_bear);
    pdiutil::safe_delete_array(m_rxQueue);
    m_rxQueueLen = 0;
}


bool TlsClientInterface::beginServer(const char* certPath, const char* keyPath, const char* clientCaPath) {
    if (m_role != ROLE_SERVER || !certPath || !keyPath) {
        LogE("TLS beginServer: bad args\n");
        return false;
    }

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    if (!startTlsWorker()) {
        LogE("TLS beginServer: startTlsWorker failed (OOM stack)\n");
        return false;
    }
    #endif

    pdiutil::safe_delete(m_bear);
    m_bear = pdiutil::safe_new<BearSSLState>();
    if (!m_bear) {
        LogE("TLS beginServer: OOM BearSSLState\n");
        return false;
    }

    if (!TlsCryptoLoader::loadCertChain(certPath, m_bear->certChain, m_bear->certChainCount, m_bear->certBacking)) {
        LogFmtE("TLS beginServer: loadCertChain failed: %s\n", certPath);
        bearReset(0);
        return false;
    }

    if (!TlsCryptoLoader::loadPrivateKey(keyPath, m_bear->skey)) {
        LogFmtE("TLS beginServer: loadPrivateKey failed: %s\n", keyPath);
        bearReset(0);
        return false;
    }

    static const uint16_t suites_rsa[] = {
        BR_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
        BR_TLS_RSA_WITH_AES_128_GCM_SHA256,
    };
    static const uint16_t suites_ec[] = {
        BR_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
    };

    int keyType = br_skey_decoder_key_type(&m_bear->skey);
    if (keyType == BR_KEYTYPE_RSA) {
        const br_rsa_private_key* rsa = br_skey_decoder_get_rsa(&m_bear->skey);
        if (!rsa) {
            LogE("TLS beginServer: get_rsa returned null\n");
            bearReset(0);
            return false;
        }
        tls_server_engine_base_init(&m_bear->sc, suites_rsa,
            sizeof(suites_rsa) / sizeof(suites_rsa[0]));
        br_ssl_server_set_single_rsa(&m_bear->sc,
            m_bear->certChain, m_bear->certChainCount,
            rsa, BR_KEYTYPE_KEYX | BR_KEYTYPE_SIGN,
            br_rsa_private_get_default(),
            br_rsa_pkcs1_sign_get_default());
    } else if (keyType == BR_KEYTYPE_EC) {
        const br_ec_private_key* ec = br_skey_decoder_get_ec(&m_bear->skey);
        if (!ec) {
            LogE("TLS beginServer: get_ec returned null\n");
            bearReset(0);
            return false;
        }
        tls_server_engine_base_init(&m_bear->sc, suites_ec,
            sizeof(suites_ec) / sizeof(suites_ec[0]));
        br_ssl_server_set_single_ec(&m_bear->sc,
            m_bear->certChain, m_bear->certChainCount,
            ec, BR_KEYTYPE_KEYX | BR_KEYTYPE_SIGN,
            BR_KEYTYPE_EC,
            br_ssl_engine_get_ec(&m_bear->sc.eng),
            br_ecdsa_i15_sign_asn1);
    } else {
        LogFmtE("TLS beginServer: unsupported key type %d\n", keyType);
        bearReset(0);
        return false;
    }

    if (!m_bear->allocIoBuffers(TLS_IBUF_SIZE, TLS_OBUF_SIZE)) {
        LogE("TLS beginServer: alloc IO buffers failed\n");
        bearReset(0);
        return false;
    }

    br_ssl_engine_set_buffers_bidi(&m_bear->sc.eng,
        m_bear->ibuf, m_bear->ibufLen,
        m_bear->obuf, m_bear->obufLen);

    if (clientCaPath && clientCaPath[0]) {
        if (!TlsCryptoLoader::loadTrustAnchors(clientCaPath,
                m_bear->trustAnchors, m_bear->trustAnchorsCount)) {
            LogFmtE("TLS beginServer: loadTrustAnchors failed: %s\n", clientCaPath);
            bearReset(0);
            return false;
        }
        tls_x509_minimal_install(&m_bear->xc,
            m_bear->trustAnchors, m_bear->trustAnchorsCount);
        br_ssl_engine_set_x509(&m_bear->sc.eng, &m_bear->xc.vtable);
        br_ssl_server_set_trust_anchor_names_alt(&m_bear->sc,
            m_bear->trustAnchors, m_bear->trustAnchorsCount);
    }

    if (!br_ssl_server_reset(&m_bear->sc)) {
        LogE("TLS beginServer: br_ssl_server_reset failed\n");
        bearReset(0);
        return false;
    }

    LogI("TLS beginServer: ready\n");
    return true;
}

#ifdef ENABLE_CONTEXTUAL_EXECUTION
bool TlsClientInterface::startTlsWorker() {
    if (m_taskId > 0 || m_taskRunning) return true;

    m_taskId = __task_scheduler.register_task([&](){
        while (m_taskRunning) {
            serviceRx();
            __i_cooperative_scheduler.sleep(TLS_TASK_POLL_MS);
        }
    });

    if (m_taskId < 0) {
        m_taskRunning = false;
    }else{
        m_taskRunning = true;
        int status = __task_scheduler.scheduleUnderExecSched(
            &__i_cooperative_scheduler, m_taskId,
            TASK_MODE_COOPERATIVE, TLS_TASK_STACK_SIZE);

        LogFmtI("TLS startTlsWorker : %d\n", status);
        if(status < 0){
            close();
        }
    }

    return m_taskRunning;
}

void TlsClientInterface::stopTlsWorker() {
    if (m_taskId < 0) return;

    m_taskRunning = false;
    int taskid = m_taskId;
    m_taskId = -1;

    LogFmtI("TLS stopTlsWorker : %d\n", taskid);

    task_t* t = __task_scheduler.get_task(taskid);
    if (t && t->_task_exec) {
        __i_cooperative_scheduler.destroy_cooperative(
            static_cast<Cooperative*>(t->_task_exec));
        t->_task_exec = nullptr;
    }
    __task_scheduler.remove_task(taskid);
}
#endif


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
        if (m_dns.in_flight) {
            // LogE("TLS connect: DNS lookup already in flight\n");
            return -98;
        }

        m_dns.in_flight = true;
        m_dns.found = false;

        err_t derr = dns_gethostbyname(hostname, &serverIp,
            &TlsClientInterface::onDnsFound, this);

        if (derr == ERR_OK) {
            m_dns.in_flight = false;
        } else if (derr == ERR_INPROGRESS) {
            while (m_dns.in_flight &&
                   (__i_dvc_ctrl.millis_now() - start) < m_timeout) {
                __i_dvc_ctrl.wait(1);
            }
            if (m_dns.in_flight) {
                LogFmtE("TLS connect: DNS timeout for %s\n", hostname);
                return -100;
            }
            if (!m_dns.found) {
                LogFmtE("TLS connect: DNS resolution failed for %s\n", hostname);
                return -100;
            }
            serverIp = m_dns.addr;
        } else {
            m_dns.in_flight = false;
            return derr;
        }
    }

    pdiutil::safe_delete(m_bear);

    m_bear = pdiutil::safe_new<BearSSLState>();
    if (!m_bear) {
        LogE("TLS connect: OOM BearSSLState\n");
        return -97;
    }

    if (m_verifyPeer && !m_caPath.empty()) {
        if (!TlsCryptoLoader::loadTrustAnchors(m_caPath.c_str(),
                m_bear->trustAnchors, m_bear->trustAnchorsCount)) {
            return bearReset(-3);
        }
    }

    static const uint16_t suites[] = {
        BR_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
        BR_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
        BR_TLS_RSA_WITH_AES_128_GCM_SHA256,
    };
    tls_client_engine_base_init(&m_bear->cc, suites,
        sizeof(suites) / sizeof(suites[0]));

    if (m_verifyPeer) {
        tls_x509_minimal_install(&m_bear->xc,
            m_bear->trustAnchors, m_bear->trustAnchorsCount);
        br_ssl_engine_set_x509(&m_bear->cc.eng, &m_bear->xc.vtable);
    } else {
        m_bear->noopX509 = pdiutil::safe_new<NoopX509State>();
        if (!m_bear->noopX509) {
            LogE("TLS connect: OOM NoopX509State\n");
            return bearReset(-97);
        }
        m_bear->noopX509->vtable = &noop_x509_vtable;
        m_bear->noopX509->is_leaf = false;
        m_bear->noopX509->have_pkey = false;
        br_ssl_engine_set_x509(&m_bear->cc.eng, &m_bear->noopX509->vtable);
    }

    if (!m_bear->allocIoBuffers(TLS_IBUF_SIZE, TLS_OBUF_SIZE)) {
        return bearReset(-97);
    }
    br_ssl_engine_set_buffers_bidi(&m_bear->cc.eng,
        m_bear->ibuf, m_bear->ibufLen,
        m_bear->obuf, m_bear->obufLen);

    if (!m_certPath.empty() && !m_keyPath.empty()) {
        if (!TlsCryptoLoader::loadCertChain(m_certPath.c_str(),
                m_bear->certChain, m_bear->certChainCount, m_bear->certBacking)) {
            return bearReset(-4);
        }
        if (!TlsCryptoLoader::loadPrivateKey(m_keyPath.c_str(), m_bear->skey)) {
            return bearReset(-4);
        }
        int kt = br_skey_decoder_key_type(&m_bear->skey);
        if (kt == BR_KEYTYPE_RSA) {
            const br_rsa_private_key* rsa = br_skey_decoder_get_rsa(&m_bear->skey);
            br_ssl_client_set_single_rsa(&m_bear->cc,
                m_bear->certChain, m_bear->certChainCount,
                rsa, br_rsa_pkcs1_sign_get_default());
        } else if (kt == BR_KEYTYPE_EC) {
            const br_ec_private_key* ec = br_skey_decoder_get_ec(&m_bear->skey);
            br_ssl_client_set_single_ec(&m_bear->cc,
                m_bear->certChain, m_bear->certChainCount,
                ec, BR_KEYTYPE_KEYX | BR_KEYTYPE_SIGN,
                BR_KEYTYPE_EC,
                br_ec_get_default(), br_ecdsa_i15_sign_asn1);
        } else {
            return bearReset(-4);
        }
    }

    const char* sni = m_sniHostname.empty() ? hostname : m_sniHostname.c_str();
    if (!br_ssl_client_reset(&m_bear->cc, sni, 0)) {
        return bearReset(-5);
    }

    m_pcb = tcp_new();
    if (!m_pcb) {
        return bearReset(-98);
    }
    tcp_arg(m_pcb, this);
    tcp_err(m_pcb, &TlsClientInterface::onError);
    tcp_recv(m_pcb, &TlsClientInterface::onReceive);
    tcp_sent(m_pcb, &TlsClientInterface::onSent);

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    if (!startTlsWorker()) {
        close();
        return -97;
    }
    #endif

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.critical_lock();
    #endif
    err_t terr = tcp_connect(m_pcb, &serverIp, port, &TlsClientInterface::onConnected);
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.critical_unlock();
    #endif
    if (terr != ERR_OK) {
        close();
        return terr < 0 ? terr : -97;
    }
    setNoDelay(true);

    while (!connected() && (__i_dvc_ctrl.millis_now() - start) < m_timeout) {
        __i_dvc_ctrl.wait(1);
    }

    if (!connected()) {
        unsigned engState = m_bear ? br_ssl_engine_current_state(&m_bear->cc.eng) : 0;
        int      engErr   = m_bear ? br_ssl_engine_last_error(&m_bear->cc.eng) : -1;
        uint32_t elapsed  = __i_dvc_ctrl.millis_now() - start;
        LogFmtE("TLS connect: timeout after %u ms (tcp.connected=%d engState=%u engErr=%d rxQ=%u)\n",
            (unsigned)elapsed, (int)m_isConnected, engState, engErr, (unsigned)m_rxQueueLen);
        close();
        return -101;
    }

    // LogFmtI("TLS connect: handshake done in %u ms\n",
    //     (unsigned)(__i_dvc_ctrl.millis_now() - start));
    return 0;
}

int16_t TlsClientInterface::disconnect() {
    return close();
}

int16_t TlsClientInterface::close() {
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    stopTlsWorker();
    #endif

    if (m_bear != nullptr && m_bear->handshakeDone && !m_bear->engineFatal) {
        br_ssl_engine_context* eng = TLS_ENG;
        br_ssl_engine_close(eng);
        pumpEngine();
    }

    if (m_rxQueue) {

        if(nullptr != m_pcb && m_rxQueueLen > 0){

            #ifdef ENABLE_CONTEXTUAL_EXECUTION
            m_mutex.critical_lock();
            #endif
            tcp_recved(m_pcb, m_rxQueueLen); // Notify the TCP stack that data has been read
            #ifdef ENABLE_CONTEXTUAL_EXECUTION
            m_mutex.critical_unlock();
            #endif
        }

        pdiutil::safe_delete_array(m_rxQueue);
        m_rxQueueLen = 0;
    }

    if (m_pcb != nullptr) {
        tcp_arg(m_pcb, nullptr);
        tcp_err(m_pcb, nullptr);
        tcp_recv(m_pcb, nullptr);
        tcp_sent(m_pcb, nullptr);
        tcp_close(m_pcb);
        m_pcb = nullptr;
    }

    m_pumpPending = false;
    m_isConnected = false;

    pdiutil::safe_delete(m_bear);
    return 0;
}

int8_t TlsClientInterface::connected() {
    #ifndef ENABLE_CONTEXTUAL_EXECUTION
    serviceRx();
    #endif
    if (!m_isConnected || !m_bear || m_bear->engineFatal) return 0;
    if (m_role == ROLE_SERVER) return 1;
    return m_bear->handshakeDone ? 1 : 0;
}

int32_t TlsClientInterface::write(const uint8_t* data, uint32_t size) {
    if (!data || size == 0) return 0;
    if (!m_bear || !m_pcb || m_bear->engineFatal) return 0;
    if (!m_bear->handshakeDone) return 0;

    br_ssl_engine_context* eng = TLS_ENG;

    uint32_t written = 0;
    while (written < size) {
        unsigned state = br_ssl_engine_current_state(eng);
        if (state & BR_SSL_CLOSED) break;
        if (!(state & BR_SSL_SENDAPP)) break;

        size_t bufLen = 0;
        unsigned char* appBuf = br_ssl_engine_sendapp_buf(eng, &bufLen);
        if (!appBuf || bufLen == 0) break;

        uint32_t toCopy = size - written;
        if (toCopy > bufLen) toCopy = bufLen;
        memcpy(appBuf, data + written, toCopy);
        br_ssl_engine_sendapp_ack(eng, toCopy);
        written += toCopy;

        br_ssl_engine_flush(eng, 0);
        pumpEngine();
    }

    return static_cast<int32_t>(written);
}

int32_t TlsClientInterface::write_ro(const char* c_str) {
    return c_str ? write(reinterpret_cast<const uint8_t*>(c_str), strlen(c_str)) : 0;
}

int32_t TlsClientInterface::read(uint8_t* buffer, uint32_t size) {
    if (!buffer || size == 0) return 0;
    #ifndef ENABLE_CONTEXTUAL_EXECUTION
    serviceRx();
    #endif
    if (!m_bear || m_bear->engineFatal) return 0;

    br_ssl_engine_context* eng = TLS_ENG;

    uint32_t total = 0;
    while (total < size) {
        unsigned state = br_ssl_engine_current_state(eng);
        if (!(state & BR_SSL_RECVAPP)) break;

        size_t bufLen = 0;
        unsigned char* appBuf = br_ssl_engine_recvapp_buf(eng, &bufLen);
        if (!appBuf || bufLen == 0) break;

        uint32_t toCopy = size - total;
        if (toCopy > bufLen) toCopy = bufLen;
        memcpy(buffer + total, appBuf, toCopy);
        br_ssl_engine_recvapp_ack(eng, toCopy);
        total += toCopy;
    }

    return static_cast<int32_t>(total);
}

int32_t TlsClientInterface::available() {
    #ifndef ENABLE_CONTEXTUAL_EXECUTION
    serviceRx();
    #endif
    if (!m_bear || m_bear->engineFatal) return 0;

    br_ssl_engine_context* eng = TLS_ENG;
    unsigned state = br_ssl_engine_current_state(eng);
    if (!(state & BR_SSL_RECVAPP)) return 0;

    size_t bufLen = 0;
    br_ssl_engine_recvapp_buf(eng, &bufLen);
    return static_cast<int32_t>(bufLen);
}

ipaddress_t TlsClientInterface::getLocalIp() const {
    return (m_pcb) ? ipaddress_t(m_pcb->local_ip.addr) : ipaddress_t(IP4_ADDRESS_NONE);
}
uint16_t TlsClientInterface::getLocalPort() const {
    return m_pcb ? m_pcb->local_port : 0;
}
ipaddress_t TlsClientInterface::getRemoteIp() const {
    return (m_pcb) ? ipaddress_t(m_pcb->remote_ip.addr) : ipaddress_t(IP4_ADDRESS_NONE);
}
uint16_t TlsClientInterface::getRemotePort() const {
    return m_pcb ? m_pcb->remote_port : 0;
}

bool TlsClientInterface::setKeepAlive(uint16_t /*idleTime*/, uint16_t /*interval*/, uint16_t /*count*/) {
    return false;
}
void TlsClientInterface::setNoDelay(bool noDelay) {
    if (!m_pcb) return;
    if (noDelay) tcp_nagle_disable(m_pcb);
    else         tcp_nagle_enable(m_pcb);
}
bool TlsClientInterface::availableforwrite(uint32_t size) {

    __i_dvc_ctrl.yield();

    if (!m_pcb || !m_isConnected) return false;

    if (m_pcb->state != ESTABLISHED &&
        m_pcb->state != CLOSE_WAIT &&
        m_pcb->state != SYN_SENT &&
        m_pcb->state != SYN_RCVD) {
        return false;
    }

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.critical_lock();
    #endif
    tcp_output(m_pcb);
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    m_mutex.critical_unlock();
    #endif

    uint32_t availablebuff = tcp_sndbuf(m_pcb);
    uint32_t queuelen = tcp_sndqueuelen(m_pcb);

    if ((availablebuff < size) || (queuelen >= TCP_SND_QUEUELEN) || (queuelen > TCP_SNDQUEUELEN_OVERFLOW)) {
        __i_dvc_ctrl.yield();
        return false;
    }

    __i_dvc_ctrl.yield();

    br_ssl_engine_context* eng = TLS_ENG;
    unsigned state = br_ssl_engine_current_state(eng);
    if (!(state & BR_SSL_SENDAPP)) return false;

    size_t bufLen = 0;
    br_ssl_engine_sendapp_buf(eng, &bufLen);
    if (bufLen < size) {
        pumpEngine();
        return false;
    }

    return m_isLastWriteAcked;
}

void TlsClientInterface::flush() {
    if (m_bear){

        br_ssl_engine_context* eng = TLS_ENG;
        br_ssl_engine_flush(eng, 0);
        pumpEngine();
    }

    if (m_pcb) tcp_output(m_pcb);
    m_isLastWriteAcked = true;
}


err_t TlsClientInterface::onConnected(void* arg, struct tcp_pcb* /*tpcb*/, err_t err) {
    TlsClientInterface* self = static_cast<TlsClientInterface*>(arg);
    if (!self) return ERR_VAL;
    if (err != ERR_OK) {
        if (self->m_bear) self->m_bear->engineFatal = true;
        return err;
    }
    self->m_isConnected = true;
    self->m_pumpPending = true;
    return ERR_OK;
}

err_t TlsClientInterface::onReceive(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    TlsClientInterface* self = static_cast<TlsClientInterface*>(arg);
    if (!self) {
        if (p) pbuf_free(p);
        return ERR_VAL;
    }

    if (err != ERR_OK || !p) {
        if (p) pbuf_free(p);
        if (err != ERR_OK && self->m_bear) self->m_bear->engineFatal = true;
        return ERR_OK;
    }

    uint32_t newSize = self->m_rxQueueLen + p->tot_len;
    if((newSize+self->m_rxQueueLen) > (0.90*TLS_IBUF_SIZE)){
        // LogFmtE("TLS onReceive: rxQ cap, in=%u rxQ=%u\n",
        //     (unsigned)p->tot_len, (unsigned)self->m_rxQueueLen);
        return ERR_MEM;
    }

    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    self->m_mutex.critical_lock();
    #endif
    uint8_t* newBuf = pdiutil::safe_new_array<uint8_t>(newSize);
    if (!newBuf) {
        #ifdef ENABLE_CONTEXTUAL_EXECUTION
        self->m_mutex.critical_unlock();
        #endif
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
    #ifdef ENABLE_CONTEXTUAL_EXECUTION
    self->m_mutex.critical_unlock();
    #endif

    pbuf_free(p);
    return ERR_OK;
}

void TlsClientInterface::onError(void* arg, err_t err) {
    TlsClientInterface* self = static_cast<TlsClientInterface*>(arg);
    if (!self) return;
    // LogFmtE("TLS onError: lwIP err=%d\n", (int)err);
    self->m_pcb = nullptr;
    self->flush();
    self->m_isConnected = false;
    if (self->m_bear) self->m_bear->engineFatal = true;
}

err_t TlsClientInterface::onSent(void* arg, struct tcp_pcb* /*tpcb*/, u16_t /*len*/) {
    TlsClientInterface* self = static_cast<TlsClientInterface*>(arg);
    if (!self) return ERR_VAL;
    self->m_isLastWriteAcked = true;
    self->m_pumpPending = true;
    return ERR_OK;
}

void TlsClientInterface::serviceRx() {
    if (!m_bear || m_bear->engineFatal) return;

    br_ssl_engine_context* eng = TLS_ENG;
    uint16_t reclaimed = 0;

    while (true) {
        #ifdef ENABLE_CONTEXTUAL_EXECUTION
        m_mutex.critical_lock();
        #endif
        if (m_rxQueueLen == 0) {
            #ifdef ENABLE_CONTEXTUAL_EXECUTION
            m_mutex.critical_unlock();
            #endif
            break;
        }

        unsigned state = br_ssl_engine_current_state(eng);
        if ((state & BR_SSL_CLOSED) || !(state & BR_SSL_RECVREC)) {
            #ifdef ENABLE_CONTEXTUAL_EXECUTION
            m_mutex.critical_unlock();
            #endif
            break;
        }

        size_t bufLen = 0;
        unsigned char* recvBuf = br_ssl_engine_recvrec_buf(eng, &bufLen);
        if (!recvBuf || bufLen == 0) {
            #ifdef ENABLE_CONTEXTUAL_EXECUTION
            m_mutex.critical_unlock();
            #endif
            break;
        }
        __i_dvc_ctrl.yield();

        uint32_t chunk = (m_rxQueueLen < bufLen) ? m_rxQueueLen : (uint32_t)bufLen;
        memcpy(recvBuf, m_rxQueue, chunk);

        if (chunk < m_rxQueueLen) {
            memmove(m_rxQueue, m_rxQueue + chunk, m_rxQueueLen - chunk);
            m_rxQueueLen -= chunk;
        } else {
            pdiutil::safe_delete_array(m_rxQueue);
            m_rxQueueLen = 0;
        }
        #ifdef ENABLE_CONTEXTUAL_EXECUTION
        m_mutex.critical_unlock();
        #endif

        br_ssl_engine_recvrec_ack(eng, chunk);
        reclaimed += chunk;

        pumpEngine();
        __i_dvc_ctrl.yield();
    }

    if (m_pcb) {
        #ifdef ENABLE_CONTEXTUAL_EXECUTION
        m_mutex.critical_lock();
        #endif
        tcp_recved(m_pcb, reclaimed);
        #ifdef ENABLE_CONTEXTUAL_EXECUTION
        m_mutex.critical_unlock();
        #endif
    }

    unsigned state = br_ssl_engine_current_state(eng);
    if ((state & BR_SSL_SENDAPP) && !m_bear->handshakeDone) {
        m_bear->handshakeDone = true;

        if (m_bear->trustAnchors) {
            if (m_role == ROLE_SERVER) {
                m_bear->sc.tas = nullptr;
                m_bear->sc.num_tas = 0;
            }
            m_bear->xc.trust_anchors = nullptr;
            m_bear->xc.trust_anchors_num = 0;
            TlsCryptoLoader::freeTrustAnchors(m_bear->trustAnchors, m_bear->trustAnchorsCount);
        }

        if (m_role == ROLE_SERVER && m_bear->certChain) {
            m_bear->sc.eng.chain = nullptr;
            m_bear->sc.eng.chain_len = 0;
            m_bear->sc.eng.cert_cur = nullptr;
            m_bear->sc.eng.cert_len = 0;
            m_bear->sc.chain_handler.single_rsa.chain = nullptr;
            m_bear->sc.chain_handler.single_rsa.chain_len = 0;
            TlsCryptoLoader::freeCertChain(m_bear->certChain, m_bear->certChainCount, m_bear->certBacking);
        }
    }

    if (state & BR_SSL_CLOSED) {
        int err = br_ssl_engine_last_error(eng);
        if (err != BR_ERR_OK) {
            LogFmtE("TLS serviceRx: engine closed with err=%d\n", err);
            m_bear->engineFatal = true;
        }
    }

    if (m_pumpPending) {
        m_pumpPending = false;
        pumpEngine();
    }

    __i_dvc_ctrl.yield();
}

void TlsClientInterface::onDnsFound(const char* /*name*/, const ip_addr_t* ipaddr, void* arg) {
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

void TlsClientInterface::pumpEngine() {
    if (m_inPump) return;
    if (!m_bear || m_bear->engineFatal || !m_pcb) return;

    m_inPump = true;

    while (true) {
        if (!m_bear || m_bear->engineFatal || !m_pcb) break;

        br_ssl_engine_context* eng = TLS_ENG;

        unsigned state = br_ssl_engine_current_state(eng);

        if (state & BR_SSL_CLOSED) {
            int err = br_ssl_engine_last_error(eng);
            if (err != BR_ERR_OK) {
                LogFmtE("TLS pumpEngine: engine closed with err=%d\n", err);
                m_bear->engineFatal = true;
            }
            break;
        }

        if (!(state & BR_SSL_SENDREC)) break;

        size_t len = 0;
        unsigned char* buf = br_ssl_engine_sendrec_buf(eng, &len);
        if (!buf || len == 0) break;

        uint16_t sndbuf = tcp_sndbuf(m_pcb);
        if (sndbuf == 0) break;

        uint16_t toSend = (len < sndbuf) ? static_cast<uint16_t>(len) : sndbuf;

        uint8_t flags = TCP_WRITE_FLAG_COPY;

        if (!m_isLastWriteAcked){
            flags |= TCP_WRITE_FLAG_MORE; // do not tcp-PuSH (yet)
        }

        #ifdef ENABLE_CONTEXTUAL_EXECUTION
        m_mutex.critical_lock();
        #endif
        err_t err = tcp_write(m_pcb, buf, toSend, flags);
        #ifdef ENABLE_CONTEXTUAL_EXECUTION
        m_mutex.critical_unlock();
        #endif

        if (m_isLastWriteAcked) {
            #ifdef ENABLE_CONTEXTUAL_EXECUTION
            m_mutex.critical_lock();
            #endif
            m_isLastWriteAcked = false;
            err = tcp_output(m_pcb);
            #ifdef ENABLE_CONTEXTUAL_EXECUTION
            m_mutex.critical_unlock();
            #endif
        }

        br_ssl_engine_sendrec_ack(eng, toSend);
        __i_dvc_ctrl.yield();

        if (err != ERR_OK) {
            LogFmtE("TLS pumpEngine: tcp_write err=%d\n", (int)err);
            if (err != ERR_MEM && m_bear) m_bear->engineFatal = true;
            break;
        }
    }

    m_inPump = false;
}

#undef TLS_ENG
