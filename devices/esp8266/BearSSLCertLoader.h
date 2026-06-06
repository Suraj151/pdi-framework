/********************** BearSSL Cert/Key Loader ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 2nd June 2026
******************************************************************************/
#ifndef _BEARSSL_CERT_LOADER_H_
#define _BEARSSL_CERT_LOADER_H_

#include "esp8266.h"
#include <bearssl/bearssl.h>

/**
 * @namespace TlsCryptoLoader
 * @brief Helpers to read PEM cert/key material from the filesystem and decode
 *        it into BearSSL structures usable by the TLS engine.
 *
 * All allocations are heap-backed and reported back to the caller; matching
 * free* helpers are provided so the caller can release resources when the
 * TLS session ends.
 *
 * These helpers assume ENABLE_STORAGE_SERVICE; the filesystem is reached via
 * the framework's iInstanceInterface.
 */
namespace TlsCryptoLoader {

/**
 * @brief Read a PEM file containing one or more X.509 certificates and produce
 *        a BearSSL cert chain.
 * @param path         Filesystem path to the PEM bundle.
 * @param outChain     Receives a heap-allocated array of br_x509_certificate.
 * @param outCount     Receives the number of certificates parsed.
 * @param outBacking   Receives the heap-allocated DER backing buffer
 *                     (cert.data pointers reference into this buffer).
 * @return True on success; both outChain and outBacking are owned by the caller.
 */
bool loadCertChain(
    const char* path,
    br_x509_certificate*& outChain,
    size_t& outCount,
    uint8_t*& outBacking
);

/**
 * @brief Release a cert chain previously produced by loadCertChain.
 */
void freeCertChain(
    br_x509_certificate*& chain,
    size_t& count,
    uint8_t*& backing
);

/**
 * @brief Read a PEM file containing a single private key and populate a
 *        BearSSL skey decoder. The decoder owns its parsed key material;
 *        the caller must keep the decoder alive for the lifetime of the
 *        key references it returns (br_skey_decoder_get_ec / get_rsa).
 * @param path        Filesystem path to the PEM-encoded private key.
 * @param outDecoder  Decoder context to populate (initialised by this call).
 * @return True if a key was decoded without error.
 */
bool loadPrivateKey(
    const char* path,
    br_skey_decoder_context& outDecoder
);

/**
 * @brief Read a PEM file containing one or more X.509 CA certificates and
 *        produce a BearSSL trust anchor array suitable for use with
 *        br_x509_minimal_init().
 * @param path        Filesystem path to the PEM-encoded CA bundle.
 * @param outAnchors  Receives a heap-allocated array of br_x509_trust_anchor.
 * @param outCount    Receives the number of anchors built.
 * @return True on success; outAnchors is owned by the caller.
 */
bool loadTrustAnchors(
    const char* path,
    br_x509_trust_anchor*& outAnchors,
    size_t& outCount
);

/**
 * @brief Release a trust anchor array previously produced by loadTrustAnchors.
 *        Frees each anchor's allocated DN and public-key buffers.
 */
void freeTrustAnchors(
    br_x509_trust_anchor*& anchors,
    size_t& count
);

} // namespace TlsCryptoLoader

#endif // _BEARSSL_CERT_LOADER_H_
