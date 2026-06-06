/********************** BearSSL Cert/Key Loader ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 2nd June 2026
******************************************************************************/
#include "BearSSLCertLoader.h"
#include "InstanceInterface.h"
#include <string.h>
#include <stdlib.h>

namespace TlsCryptoLoader {

namespace {

constexpr uint32_t PEM_READ_CHUNK = 256;

struct CertCountCtx {
    size_t certCount;
    size_t totalDerBytes;
    size_t currentObjBytes;
};

struct CertFillCtx {
    uint8_t* backing;
    size_t   backingPos;
    br_x509_certificate* chain;
    size_t   chainCap;
    size_t   chainPos;
    size_t   currentObjStart;
};

static void cert_count_sink(void* dest_ctx, const void* /*src*/, size_t len) {
    static_cast<CertCountCtx*>(dest_ctx)->currentObjBytes += len;
}

static void cert_fill_sink(void* dest_ctx, const void* src, size_t len) {
    CertFillCtx* c = static_cast<CertFillCtx*>(dest_ctx);
    memcpy(c->backing + c->backingPos, src, len);
    c->backingPos += len;
}

static void skey_decoder_sink(void* dest_ctx, const void* src, size_t len) {
    br_skey_decoder_push(static_cast<br_skey_decoder_context*>(dest_ctx), src, len);
}

static bool name_is_certificate(const char* name) {
    return name != nullptr && strstr(name, "CERTIFICATE") != nullptr;
}

static bool name_is_private_key(const char* name) {
    return name != nullptr && strstr(name, "PRIVATE KEY") != nullptr;
}

static bool stream_pem(const char* path,
                       br_pem_decoder_context& pd,
                       pdiutil::function<bool(int evt, const char* name)> onEvent) {

    if (!path) return false;
    if (!__i_instance.getFileSystemInstance().isFileExist(path)) return false;

    bool streamOk = true;
    int rc = __i_instance.getFileSystemInstance().readFile(
        path, PEM_READ_CHUNK,
        [&](char* data, uint32_t size) -> bool {
            const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
            size_t remaining = size;
            while (remaining > 0) {
                size_t pushed = br_pem_decoder_push(&pd, p, remaining);
                p += pushed;
                remaining -= pushed;

                int evt = br_pem_decoder_event(&pd);
                if (evt == 0) {
                    if (pushed == 0) break;
                    continue;
                }

                const char* name = (evt == BR_PEM_BEGIN_OBJ)
                    ? br_pem_decoder_name(&pd) : nullptr;

                if (!onEvent(evt, name)) {
                    streamOk = false;
                    return false;
                }

                if (evt == BR_PEM_ERROR) {
                    streamOk = false;
                    return false;
                }
            }
            __i_instance.getUtilityInstance().yield();
            return true;
        });

    return streamOk && rc >= 0;
}

} // anonymous namespace

bool loadCertChain(const char* path,
                   br_x509_certificate*& outChain,
                   size_t& outCount,
                   uint8_t*& outBacking) {

    outChain = nullptr;
    outCount = 0;
    outBacking = nullptr;

    br_pem_decoder_context pd;
    br_pem_decoder_init(&pd);

    CertCountCtx countCtx{0, 0, 0};
    bool ok = stream_pem(path, pd,
        [&](int evt, const char* name) -> bool {
            if (evt == BR_PEM_BEGIN_OBJ) {
                countCtx.currentObjBytes = 0;
                if (name_is_certificate(name)) {
                    br_pem_decoder_setdest(&pd, &cert_count_sink, &countCtx);
                } else {
                    br_pem_decoder_setdest(&pd, nullptr, nullptr);
                }
            } else if (evt == BR_PEM_END_OBJ) {
                if (countCtx.currentObjBytes > 0) {
                    countCtx.certCount++;
                    countCtx.totalDerBytes += countCtx.currentObjBytes;
                    countCtx.currentObjBytes = 0;
                }
            }
            return true;
        });

    if (!ok || countCtx.certCount == 0 || countCtx.totalDerBytes == 0) {
        return false;
    }

    uint8_t* backing = static_cast<uint8_t*>(malloc(countCtx.totalDerBytes));
    br_x509_certificate* chain = static_cast<br_x509_certificate*>(
        malloc(countCtx.certCount * sizeof(br_x509_certificate)));

    if (!backing || !chain) {
        free(backing);
        free(chain);
        return false;
    }

    br_pem_decoder_init(&pd);
    CertFillCtx fillCtx{backing, 0, chain, countCtx.certCount, 0, 0};

    ok = stream_pem(path, pd,
        [&](int evt, const char* name) -> bool {
            if (evt == BR_PEM_BEGIN_OBJ) {
                if (name_is_certificate(name) && fillCtx.chainPos < fillCtx.chainCap) {
                    fillCtx.currentObjStart = fillCtx.backingPos;
                    br_pem_decoder_setdest(&pd, &cert_fill_sink, &fillCtx);
                } else {
                    br_pem_decoder_setdest(&pd, nullptr, nullptr);
                }
            } else if (evt == BR_PEM_END_OBJ) {
                if (fillCtx.backingPos > fillCtx.currentObjStart) {
                    chain[fillCtx.chainPos].data = backing + fillCtx.currentObjStart;
                    chain[fillCtx.chainPos].data_len = fillCtx.backingPos - fillCtx.currentObjStart;
                    fillCtx.chainPos++;
                    fillCtx.currentObjStart = fillCtx.backingPos;
                }
            }
            return true;
        });

    if (!ok || fillCtx.chainPos != countCtx.certCount) {
        free(backing);
        free(chain);
        return false;
    }

    outChain = chain;
    outCount = countCtx.certCount;
    outBacking = backing;
    return true;
}

void freeCertChain(br_x509_certificate*& chain, size_t& count, uint8_t*& backing) {
    free(chain);
    free(backing);
    chain = nullptr;
    backing = nullptr;
    count = 0;
}

namespace {

struct DnCapture {
    uint8_t* data;
    size_t   len;
    size_t   cap;
    bool     oom;
};

static void dn_capture_sink(void* dest_ctx, const void* src, size_t len) {
    DnCapture* dc = static_cast<DnCapture*>(dest_ctx);
    if (dc->oom) return;
    if (dc->len + len > dc->cap) {
        size_t newCap = dc->cap ? dc->cap * 2 : 256;
        while (newCap < dc->len + len) newCap *= 2;
        uint8_t* nb = static_cast<uint8_t*>(realloc(dc->data, newCap));
        if (!nb) { dc->oom = true; return; }
        dc->data = nb;
        dc->cap = newCap;
    }
    memcpy(dc->data + dc->len, src, len);
    dc->len += len;
}

static void free_ta_contents(br_x509_trust_anchor* ta) {
    if (!ta) return;
    free(ta->dn.data);
    if (ta->pkey.key_type == BR_KEYTYPE_RSA) {
        free(ta->pkey.key.rsa.n);
        free(ta->pkey.key.rsa.e);
    } else if (ta->pkey.key_type == BR_KEYTYPE_EC) {
        free(ta->pkey.key.ec.q);
    }
    memset(ta, 0, sizeof(*ta));
}

static bool cert_to_trust_anchor(br_x509_trust_anchor* ta, const br_x509_certificate* xc) {
    memset(ta, 0, sizeof(*ta));

    DnCapture dn{nullptr, 0, 0, false};
    br_x509_decoder_context dc;
    br_x509_decoder_init(&dc, &dn_capture_sink, &dn, nullptr, nullptr);
    br_x509_decoder_push(&dc, xc->data, xc->data_len);

    if (dn.oom) { free(dn.data); return false; }

    br_x509_pkey* pk = br_x509_decoder_get_pkey(&dc);
    if (!pk) { free(dn.data); return false; }

    if (dn.len < dn.cap) {
        uint8_t* trimmed = static_cast<uint8_t*>(realloc(dn.data, dn.len));
        if (trimmed) dn.data = trimmed;
    }
    ta->dn.data = dn.data;
    ta->dn.len  = dn.len;
    ta->flags   = br_x509_decoder_isCA(&dc) ? BR_X509_TA_CA : 0;

    if (pk->key_type == BR_KEYTYPE_RSA) {
        ta->pkey.key_type = BR_KEYTYPE_RSA;
        ta->pkey.key.rsa.nlen = pk->key.rsa.nlen;
        ta->pkey.key.rsa.elen = pk->key.rsa.elen;
        ta->pkey.key.rsa.n = static_cast<uint8_t*>(malloc(pk->key.rsa.nlen));
        ta->pkey.key.rsa.e = static_cast<uint8_t*>(malloc(pk->key.rsa.elen));
        if (!ta->pkey.key.rsa.n || !ta->pkey.key.rsa.e) { free_ta_contents(ta); return false; }
        memcpy(ta->pkey.key.rsa.n, pk->key.rsa.n, pk->key.rsa.nlen);
        memcpy(ta->pkey.key.rsa.e, pk->key.rsa.e, pk->key.rsa.elen);
        return true;
    }
    if (pk->key_type == BR_KEYTYPE_EC) {
        ta->pkey.key_type = BR_KEYTYPE_EC;
        ta->pkey.key.ec.curve = pk->key.ec.curve;
        ta->pkey.key.ec.qlen = pk->key.ec.qlen;
        ta->pkey.key.ec.q = static_cast<uint8_t*>(malloc(pk->key.ec.qlen));
        if (!ta->pkey.key.ec.q) { free_ta_contents(ta); return false; }
        memcpy(ta->pkey.key.ec.q, pk->key.ec.q, pk->key.ec.qlen);
        return true;
    }
    free_ta_contents(ta);
    return false;
}

} // anonymous namespace

bool loadTrustAnchors(const char* path,
                      br_x509_trust_anchor*& outAnchors,
                      size_t& outCount) {
    outAnchors = nullptr;
    outCount = 0;

    br_x509_certificate* chain = nullptr;
    size_t chainCount = 0;
    uint8_t* chainBacking = nullptr;
    if (!loadCertChain(path, chain, chainCount, chainBacking)) return false;

    br_x509_trust_anchor* anchors = static_cast<br_x509_trust_anchor*>(
        malloc(chainCount * sizeof(br_x509_trust_anchor)));
    if (!anchors) {
        freeCertChain(chain, chainCount, chainBacking);
        return false;
    }

    for (size_t i = 0; i < chainCount; i++) {
        if (!cert_to_trust_anchor(&anchors[i], &chain[i])) {
            for (size_t j = 0; j < i; j++) free_ta_contents(&anchors[j]);
            free(anchors);
            freeCertChain(chain, chainCount, chainBacking);
            return false;
        }
        __i_instance.getUtilityInstance().yield();
    }

    freeCertChain(chain, chainCount, chainBacking);
    outAnchors = anchors;
    outCount = chainCount;
    return true;
}

void freeTrustAnchors(br_x509_trust_anchor*& anchors, size_t& count) {
    if (anchors) {
        for (size_t i = 0; i < count; i++) free_ta_contents(&anchors[i]);
        free(anchors);
    }
    anchors = nullptr;
    count = 0;
}

bool loadPrivateKey(const char* path, br_skey_decoder_context& outDecoder) {

    br_pem_decoder_context pd;
    br_pem_decoder_init(&pd);
    br_skey_decoder_init(&outDecoder);

    bool sawKey = false;
    bool ok = stream_pem(path, pd,
        [&](int evt, const char* name) -> bool {
            if (evt == BR_PEM_BEGIN_OBJ) {
                if (name_is_private_key(name)) {
                    br_pem_decoder_setdest(&pd, &skey_decoder_sink, &outDecoder);
                    sawKey = true;
                } else {
                    br_pem_decoder_setdest(&pd, nullptr, nullptr);
                }
            }
            return true;
        });

    if (!ok || !sawKey) return false;
    return br_skey_decoder_last_error(&outDecoder) == 0;
}

} // namespace TlsCryptoLoader
