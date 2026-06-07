/********************** mbedTLS Cert/Key Loader ********************************
This file is part of the pdi stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 7th June 2026
******************************************************************************/
#include "MbedTLSCertLoader.h"
#include "InstanceInterface.h"
#include <stdlib.h>
#include <string.h>

namespace TlsCryptoLoader {

namespace {

static bool readFileFull(const char* path, uint8_t*& outBuf, size_t& outLen) {

    outBuf = nullptr;
    outLen = 0;
    if (!path) return false;

    iFileSystemInterface& fs = __i_instance.getFileSystemInstance();
    if (!fs.isFileExist(path)) return false;

    pdiutil::vector<uint8_t> accum;
    accum.reserve(1024);

    int rc = fs.readFile(path, 256,
        [&](char* data, uint32_t size) -> bool {
            for (uint32_t i = 0; i < size; ++i) {
                accum.push_back(static_cast<uint8_t>(data[i]));
            }
            __i_instance.getUtilityInstance().yield();
            return true;
        });

    if (rc < 0 || accum.empty()) return false;

    size_t len = accum.size();
    uint8_t* buf = static_cast<uint8_t*>(malloc(len + 1));
    if (!buf) return false;

    for (size_t i = 0; i < len; ++i) buf[i] = accum[i];
    buf[len] = 0;

    outBuf = buf;
    outLen = len + 1;
    return true;
}

}

bool loadCertChain(const char* path, mbedtls_x509_crt* chain) {

    if (!chain) return false;

    uint8_t* buf = nullptr;
    size_t   len = 0;
    if (!readFileFull(path, buf, len)) return false;

    int rc = mbedtls_x509_crt_parse(chain, buf, len);
    free(buf);
    return rc == 0;
}

bool loadTrustAnchors(const char* path, mbedtls_x509_crt* anchors) {

    return loadCertChain(path, anchors);
}

bool loadPrivateKey(const char* path,
                    mbedtls_pk_context* key,
                    mbedtls_ctr_drbg_context* rng) {

    if (!key || !rng) return false;

    uint8_t* buf = nullptr;
    size_t   len = 0;
    if (!readFileFull(path, buf, len)) return false;

    int rc = mbedtls_pk_parse_key(key, buf, len, nullptr, 0,
                                  mbedtls_ctr_drbg_random, rng);
    free(buf);
    return rc == 0;
}

}
