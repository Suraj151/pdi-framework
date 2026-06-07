/********************** mbedTLS Cert/Key Loader ********************************
This file is part of the pdi stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 7th June 2026
******************************************************************************/
#ifndef _MBEDTLS_CERT_LOADER_H_
#define _MBEDTLS_CERT_LOADER_H_

#include "esp32.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"
#include "mbedtls/ctr_drbg.h"

namespace TlsCryptoLoader {

bool loadCertChain(const char* path, mbedtls_x509_crt* chain);

bool loadTrustAnchors(const char* path, mbedtls_x509_crt* anchors);

bool loadPrivateKey(const char* path,
                    mbedtls_pk_context* key,
                    mbedtls_ctr_drbg_context* rng);

}

#endif
