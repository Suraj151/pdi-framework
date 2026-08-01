/******************************** rsa util ***********************************
This file is part of the pdi stack.

RSASSA-PKCS1-v1_5 sign/verify (rsa-sha2-256 / rsa-sha2-512) plus on-device key
generation, built on the portable bignum core. CRT is used for signing.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Aug 2026
******************************************************************************/

#ifndef _CRYPTO_RSA_H_
#define _CRYPTO_RSA_H_

#include "bignum.h"

// Public exponent used for generated keys (F4).
#define RSA_PUBLIC_EXPONENT 65537u

enum rsa_hash_alg {
    RSA_HASH_SHA256 = 0,
    RSA_HASH_SHA512
};

struct rsa_key {
    bignum n;    // modulus
    bignum e;    // public exponent
    bignum d;    // private exponent
    bignum p;    // prime 1
    bignum q;    // prime 2
    bignum dp;   // d mod (p-1)
    bignum dq;   // d mod (q-1)
    bignum qinv; // q^{-1} mod p
    bool has_private;
    bool has_crt;
};

void rsa_key_init(rsa_key *key);

// Generate an RSA key pair of `bits` (e.g. 2048). Fills all fields incl CRT.
bool rsa_generate_keypair(rsa_key *key, int32_t bits, bn_rng_fn rng);

// RSASSA-PKCS1-v1_5 signature over `msg` (hashed internally with `alg`).
// sig_out must hold at least modulus-byte-length; *sig_len returns bytes written.
bool rsa_sign_pkcs1(const rsa_key *key, rsa_hash_alg alg,
                    const uint8_t *msg, size_t msg_len,
                    uint8_t *sig_out, size_t *sig_len);

// Verify an RSASSA-PKCS1-v1_5 signature over `msg` using the public key.
bool rsa_verify_pkcs1(const rsa_key *key, rsa_hash_alg alg,
                      const uint8_t *msg, size_t msg_len,
                      const uint8_t *sig, size_t sig_len);

#endif // _CRYPTO_RSA_H_
