/********************** TLS Cert Provisioner ***********************************
This file is part of the pdi stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 7th June 2026
******************************************************************************/
#include "TlsCertProvisioner.h"
#include "InstanceInterface.h"
#include "MbedTLSCertLoader.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"
#include "mbedtls/ecp.h"
#include "mbedtls/rsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/oid.h"
#include "mbedtls/asn1.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

namespace TlsCertProvisioner {

namespace {

constexpr size_t PEM_BUF_SIZE = 4096;
constexpr size_t SAN_BUF_SIZE = 256;
constexpr size_t CN_BUF_SIZE  = 96;

static bool sanMatches(mbedtls_x509_crt* crt, uint32_t ip_v4, const char* dns_name) {

    if (!crt) return false;

    bool needIp  = (ip_v4 != 0);
    bool needDns = (dns_name && dns_name[0]);
    if (!needIp && !needDns) return true;

    size_t dnsLen = needDns ? strlen(dns_name) : 0;
    bool foundIp = false;
    bool foundDns = false;

    const mbedtls_x509_sequence* cur = &crt->subject_alt_names;
    while (cur != nullptr) {

        unsigned char tagClass = cur->buf.tag & MBEDTLS_ASN1_TAG_CLASS_MASK;
        unsigned char tagNum   = cur->buf.tag & MBEDTLS_ASN1_TAG_VALUE_MASK;

        if (tagClass == MBEDTLS_ASN1_CONTEXT_SPECIFIC && tagNum == 7) {
            if (needIp && cur->buf.len == 4) {
                uint32_t ip = (uint32_t)cur->buf.p[0]
                            | ((uint32_t)cur->buf.p[1] << 8)
                            | ((uint32_t)cur->buf.p[2] << 16)
                            | ((uint32_t)cur->buf.p[3] << 24);
                if (ip == ip_v4) foundIp = true;
            }
        } else if (tagClass == MBEDTLS_ASN1_CONTEXT_SPECIFIC && tagNum == 2) {
            if (needDns && cur->buf.len == dnsLen &&
                memcmp(cur->buf.p, dns_name, dnsLen) == 0) {
                foundDns = true;
            }
        }
        cur = cur->next;
    }

    return (!needIp || foundIp) && (!needDns || foundDns);
}

static size_t encodeSanExtension(uint8_t* out, size_t outCap,
                                  uint32_t ip_v4, const char* dns_name) {

    bool needIp  = (ip_v4 != 0);
    bool needDns = (dns_name && dns_name[0]);
    size_t dnsLen = needDns ? strlen(dns_name) : 0;

    size_t innerLen = (needIp ? 6u : 0u) + (needDns ? (2u + dnsLen) : 0u);
    if (innerLen == 0 || innerLen > 127) return 0;

    size_t totalLen = 2 + innerLen;
    if (totalLen > outCap) return 0;

    size_t pos = 0;
    out[pos++] = 0x30;
    out[pos++] = (uint8_t)innerLen;

    if (needIp) {
        out[pos++] = 0x87;
        out[pos++] = 0x04;
        out[pos++] = (uint8_t)(ip_v4 & 0xFF);
        out[pos++] = (uint8_t)((ip_v4 >> 8) & 0xFF);
        out[pos++] = (uint8_t)((ip_v4 >> 16) & 0xFF);
        out[pos++] = (uint8_t)((ip_v4 >> 24) & 0xFF);
    }

    if (needDns) {
        out[pos++] = 0x82;
        out[pos++] = (uint8_t)dnsLen;
        memcpy(out + pos, dns_name, dnsLen);
        pos += dnsLen;
    }

    return pos;
}

static mbedtls_ecp_group_id ecGroupForSize(int keyBits) {
    switch (keyBits) {
        case 256: return MBEDTLS_ECP_DP_SECP256R1;
        case 384: return MBEDTLS_ECP_DP_SECP384R1;
        case 521: return MBEDTLS_ECP_DP_SECP521R1;
        default:  return MBEDTLS_ECP_DP_SECP256R1;
    }
}

static void writeYear(char* out, uint32_t year_offset_days, uint32_t base_year) {
    snprintf(out, 15, "%04u0101000000", (unsigned)base_year);
}

}

bool generateCert(const char* certPath, const char* keyPath, const CertParams& params) {

    if (!certPath || !keyPath) return false;
    if (params.ip_v4 == 0 && (!params.dns_name || !params.dns_name[0]) &&
        (!params.subjectCn || !params.subjectCn[0])) {
        LogE("CertProvisioner: identity required\n");
        return false;
    }

    mbedtls_pk_context        key;
    mbedtls_entropy_context   entropy;
    mbedtls_ctr_drbg_context  rng;
    mbedtls_x509write_cert    crt;

    mbedtls_pk_init(&key);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&rng);
    mbedtls_x509write_crt_init(&crt);

    uint8_t* certPem = nullptr;
    uint8_t* keyPem  = nullptr;
    bool ok = false;
    int rc = 0;

    rc = mbedtls_ctr_drbg_seed(&rng, mbedtls_entropy_func, &entropy, nullptr, 0);
    if (rc != 0) { LogFmtE("CertProvisioner: ctr_drbg_seed=%d\n", rc); goto done; }

    if (params.algo == KEY_ALGO_RSA) {
        rc = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
        if (rc != 0) { LogFmtE("CertProvisioner: pk_setup_rsa=%d\n", rc); goto done; }
        int bits = (params.keySize > 0) ? params.keySize : 2048;
        rc = mbedtls_rsa_gen_key(mbedtls_pk_rsa(key),
                                  mbedtls_ctr_drbg_random, &rng,
                                  bits, 65537);
        if (rc != 0) { LogFmtE("CertProvisioner: rsa_gen_key=%d\n", rc); goto done; }
    } else {
        rc = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
        if (rc != 0) { LogFmtE("CertProvisioner: pk_setup_ec=%d\n", rc); goto done; }
        rc = mbedtls_ecp_gen_key(ecGroupForSize(params.keySize),
                                  mbedtls_pk_ec(key),
                                  mbedtls_ctr_drbg_random, &rng);
        if (rc != 0) { LogFmtE("CertProvisioner: ecp_gen_key=%d\n", rc); goto done; }
    }

    char cnBuf[CN_BUF_SIZE];
    char dnBuf[CN_BUF_SIZE + 8];

    if (params.subjectCn && params.subjectCn[0]) {
        snprintf(cnBuf, sizeof(cnBuf), "%s", params.subjectCn);
    } else if (params.dns_name && params.dns_name[0]) {
        snprintf(cnBuf, sizeof(cnBuf), "%s", params.dns_name);
    } else {
        snprintf(cnBuf, sizeof(cnBuf), "%u.%u.%u.%u",
            (unsigned)(params.ip_v4 & 0xFF),
            (unsigned)((params.ip_v4 >> 8) & 0xFF),
            (unsigned)((params.ip_v4 >> 16) & 0xFF),
            (unsigned)((params.ip_v4 >> 24) & 0xFF));
    }
    snprintf(dnBuf, sizeof(dnBuf), "CN=%s", cnBuf);

    rc = mbedtls_x509write_crt_set_subject_name(&crt, dnBuf);
    if (rc != 0) { LogFmtE("CertProvisioner: set_subject=%d\n", rc); goto done; }

    if (params.issuerCn && params.issuerCn[0]) {
        char issuerDn[CN_BUF_SIZE + 8];
        snprintf(issuerDn, sizeof(issuerDn), "CN=%s", params.issuerCn);
        rc = mbedtls_x509write_crt_set_issuer_name(&crt, issuerDn);
    } else {
        rc = mbedtls_x509write_crt_set_issuer_name(&crt, dnBuf);
    }
    if (rc != 0) { LogFmtE("CertProvisioner: set_issuer=%d\n", rc); goto done; }

    mbedtls_x509write_crt_set_subject_key(&crt, &key);
    mbedtls_x509write_crt_set_issuer_key(&crt, &key);
    mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);

    {
        unsigned char serialBytes[1] = { 0x01 };
        rc = mbedtls_x509write_crt_set_serial_raw(&crt, serialBytes, sizeof(serialBytes));
        if (rc != 0) { LogFmtE("CertProvisioner: set_serial=%d\n", rc); goto done; }
    }

    rc = mbedtls_x509write_crt_set_validity(&crt,
            "20200101000000", "20990101000000");
    if (rc != 0) { LogFmtE("CertProvisioner: set_validity=%d\n", rc); goto done; }

    rc = mbedtls_x509write_crt_set_basic_constraints(&crt,
            params.isCa ? 1 : 0,
            params.isCa ? 0 : -1);
    if (rc != 0) { LogFmtE("CertProvisioner: set_bc=%d\n", rc); goto done; }

    rc = mbedtls_x509write_crt_set_key_usage(&crt,
            MBEDTLS_X509_KU_DIGITAL_SIGNATURE | MBEDTLS_X509_KU_KEY_ENCIPHERMENT);
    if (rc != 0) { LogFmtE("CertProvisioner: set_ku=%d\n", rc); goto done; }

    if (params.ip_v4 != 0 || (params.dns_name && params.dns_name[0])) {
        uint8_t sanBuf[SAN_BUF_SIZE];
        size_t sanLen = encodeSanExtension(sanBuf, sizeof(sanBuf),
                                            params.ip_v4, params.dns_name);
        if (sanLen == 0) {
            LogE("CertProvisioner: SAN encode failed\n");
            goto done;
        }
        rc = mbedtls_x509write_crt_set_extension(&crt,
                MBEDTLS_OID_SUBJECT_ALT_NAME,
                MBEDTLS_OID_SIZE(MBEDTLS_OID_SUBJECT_ALT_NAME),
                0, sanBuf, sanLen);
        if (rc != 0) { LogFmtE("CertProvisioner: set_san=%d\n", rc); goto done; }
    }

    certPem = static_cast<uint8_t*>(malloc(PEM_BUF_SIZE));
    keyPem  = static_cast<uint8_t*>(malloc(PEM_BUF_SIZE));
    if (!certPem || !keyPem) { LogE("CertProvisioner: OOM PEM bufs\n"); goto done; }

    memset(certPem, 0, PEM_BUF_SIZE);
    memset(keyPem,  0, PEM_BUF_SIZE);

    rc = mbedtls_x509write_crt_pem(&crt, certPem, PEM_BUF_SIZE,
                                    mbedtls_ctr_drbg_random, &rng);
    if (rc != 0) { LogFmtE("CertProvisioner: crt_pem=%d\n", rc); goto done; }

    rc = mbedtls_pk_write_key_pem(&key, keyPem, PEM_BUF_SIZE);
    if (rc != 0) { LogFmtE("CertProvisioner: key_pem=%d\n", rc); goto done; }

    {
        iFileSystemInterface& fs = __i_instance.getFileSystemInstance();
        int w1 = fs.writeFile(certPath, (const char*)certPem, strlen((const char*)certPem), false);
        if (w1 < 0) { LogFmtE("CertProvisioner: writeFile cert=%d\n", w1); goto done; }
        int w2 = fs.writeFile(keyPath, (const char*)keyPem, strlen((const char*)keyPem), false);
        if (w2 < 0) { LogFmtE("CertProvisioner: writeFile key=%d\n", w2); goto done; }
    }

    ok = true;
    LogI("CertProvisioner: generated new cert + key\n");

done:
    if (certPem) free(certPem);
    if (keyPem)  free(keyPem);
    mbedtls_x509write_crt_free(&crt);
    mbedtls_ctr_drbg_free(&rng);
    mbedtls_entropy_free(&entropy);
    mbedtls_pk_free(&key);
    return ok;
}

bool ensureServerCert(const char* certPath, const char* keyPath,
                      uint32_t ip_v4, const char* dns_name) {

    if (!certPath || !keyPath) return false;
    if (ip_v4 == 0 && (!dns_name || !dns_name[0])) return false;

    iFileSystemInterface& fs = __i_instance.getFileSystemInstance();
    bool certExists = fs.isFileExist(certPath);
    bool keyExists  = fs.isFileExist(keyPath);

    if (certExists && keyExists) {
        mbedtls_x509_crt crt;
        mbedtls_x509_crt_init(&crt);
        bool loaded = TlsCryptoLoader::loadCertChain(certPath, &crt);
        bool matched = loaded && sanMatches(&crt, ip_v4, dns_name);
        mbedtls_x509_crt_free(&crt);
        if (matched) {
            LogI("CertProvisioner: existing server cert is current\n");
            return true;
        }
    }

    CertParams p;
    p.algo = KEY_ALGO_EC;
    p.keySize = 256;
    p.ip_v4 = ip_v4;
    p.dns_name = dns_name;
    return generateCert(certPath, keyPath, p);
}

}
