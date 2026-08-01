/******************************** rsa util ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Aug 2026
******************************************************************************/

#include "rsa.h"
#include <utility/SafeAlloc.h>
#include <utility/crypto/hash/sha256.h>
#include <utility/crypto/hash/sha512.h>
#include <string.h>

// EMSA-PKCS1-v1_5 DigestInfo prefixes (ASN.1) per RFC 8017.
static const uint8_t s_digestinfo_sha256[] = {
    0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65,
    0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20
};
static const uint8_t s_digestinfo_sha512[] = {
    0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65,
    0x03, 0x04, 0x02, 0x03, 0x05, 0x00, 0x04, 0x40
};

static bool hash_msg(rsa_hash_alg alg, const uint8_t *msg, size_t msg_len,
                     uint8_t *out, size_t *hash_len,
                     const uint8_t **prefix, size_t *prefix_len) {
    if (alg == RSA_HASH_SHA256) {
        sha256(msg, (unsigned int)msg_len, out);
        *hash_len = 32;
        *prefix = s_digestinfo_sha256;
        *prefix_len = sizeof(s_digestinfo_sha256);
        return true;
    } else if (alg == RSA_HASH_SHA512) {
        sha512(msg, (unsigned int)msg_len, out);
        *hash_len = 64;
        *prefix = s_digestinfo_sha512;
        *prefix_len = sizeof(s_digestinfo_sha512);
        return true;
    }
    return false;
}

// Build EM = 0x00 || 0x01 || PS(0xFF..) || 0x00 || DigestInfo || H, length k.
static bool build_emsa_pkcs1(rsa_hash_alg alg, const uint8_t *msg, size_t msg_len,
                             uint8_t *em, size_t k) {
    uint8_t hash[64];
    size_t hash_len = 0, prefix_len = 0;
    const uint8_t *prefix = nullptr;
    if (!hash_msg(alg, msg, msg_len, hash, &hash_len, &prefix, &prefix_len)) return false;

    size_t t_len = prefix_len + hash_len;
    if (k < t_len + 11) return false; // PS must be >= 8 bytes

    size_t ps_len = k - t_len - 3;
    size_t off = 0;
    em[off++] = 0x00;
    em[off++] = 0x01;
    for (size_t i = 0; i < ps_len; i++) em[off++] = 0xFF;
    em[off++] = 0x00;
    memcpy(em + off, prefix, prefix_len); off += prefix_len;
    memcpy(em + off, hash, hash_len); off += hash_len;
    return off == k;
}

void rsa_key_init(rsa_key *key) {
    bn_zero(&key->n);
    bn_zero(&key->e);
    bn_zero(&key->d);
    bn_zero(&key->p);
    bn_zero(&key->q);
    bn_zero(&key->dp);
    bn_zero(&key->dq);
    bn_zero(&key->qinv);
    key->has_private = false;
    key->has_crt = false;
}

bool rsa_generate_keypair(rsa_key *key, int32_t bits, bn_rng_fn rng) {
    if (bits < 512 || bits > RSA_MAX_KEY_BITS) return false;
    rsa_key_init(key);
    bn_set_u32(&key->e, RSA_PUBLIC_EXPONENT);

    bignum *p1 = pdiutil::safe_new<bignum>();
    bignum *q1 = pdiutil::safe_new<bignum>();
    bignum *lambda = pdiutil::safe_new<bignum>();
    bignum *g = pdiutil::safe_new<bignum>();
    bignum *tmp = pdiutil::safe_new<bignum>();
    bignum *one = pdiutil::safe_new<bignum>();
    if (!p1 || !q1 || !lambda || !g || !tmp || !one) {
        pdiutil::safe_delete(p1); pdiutil::safe_delete(q1);
        pdiutil::safe_delete(lambda); pdiutil::safe_delete(g);
        pdiutil::safe_delete(tmp); pdiutil::safe_delete(one);
        return false;
    }
    bn_set_u32(one, 1);

    bool ok = false;
    int32_t half = bits / 2;
    for (int32_t attempt = 0; attempt < 64 && !ok; attempt++) {
        if (!bn_gen_prime(&key->p, half, rng)) break;
        if (!bn_gen_prime(&key->q, bits - half, rng)) break;
        if (bn_cmp(&key->p, &key->q) == 0) continue;

        // n = p * q, verify exact bit length
        if (!bn_mul(&key->n, &key->p, &key->q)) continue;
        if (bn_bitlen(&key->n) != bits) continue;

        // lambda = lcm(p-1, q-1) = (p-1)*(q-1)/gcd(p-1,q-1)
        bn_sub(p1, &key->p, one);
        bn_sub(q1, &key->q, one);
        if (!bn_gcd(g, p1, q1)) continue;
        if (!bn_mul(tmp, p1, q1)) continue;
        if (!bn_divmod(tmp, g, lambda, nullptr)) continue;

        // require gcd(e, lambda) == 1
        if (!bn_gcd(g, &key->e, lambda)) continue;
        if (bn_cmp(g, one) != 0) continue;

        // d = e^{-1} mod lambda
        if (!bn_modinv(&key->d, &key->e, lambda)) continue;

        // CRT params
        if (!bn_mod(&key->dp, &key->d, p1)) continue;
        if (!bn_mod(&key->dq, &key->d, q1)) continue;
        if (!bn_modinv(&key->qinv, &key->q, &key->p)) continue;

        key->has_private = true;
        key->has_crt = true;
        ok = true;
    }

    pdiutil::safe_delete(p1); pdiutil::safe_delete(q1);
    pdiutil::safe_delete(lambda); pdiutil::safe_delete(g);
    pdiutil::safe_delete(tmp); pdiutil::safe_delete(one);
    return ok;
}

// s = m^d mod n via CRT when available, else plain modexp.
static bool rsa_private_op(const rsa_key *key, const bignum *m, bignum *s) {
    if (!key->has_crt) {
        return bn_modexp(s, m, &key->d, &key->n);
    }

    bignum *m1 = pdiutil::safe_new<bignum>();
    bignum *m2 = pdiutil::safe_new<bignum>();
    bignum *h = pdiutil::safe_new<bignum>();
    bignum *t = pdiutil::safe_new<bignum>();
    if (!m1 || !m2 || !h || !t) {
        pdiutil::safe_delete(m1); pdiutil::safe_delete(m2);
        pdiutil::safe_delete(h); pdiutil::safe_delete(t);
        return false;
    }

    bool ok = true;
    ok = ok && bn_modexp(m1, m, &key->dp, &key->p); // m^dp mod p
    ok = ok && bn_modexp(m2, m, &key->dq, &key->q); // m^dq mod q

    if (ok) {
        // h = qinv * (m1 - m2) mod p
        if (bn_cmp(m1, m2) >= 0) {
            bn_sub(t, m1, m2);
        } else {
            bn_add(t, m1, &key->p);
            bn_sub(t, t, m2);
        }
        ok = bn_mulmod(h, &key->qinv, t, &key->p);
    }
    if (ok) {
        // s = m2 + h * q
        ok = bn_mul(t, h, &key->q);
        if (ok) ok = bn_add(s, m2, t);
    }

    pdiutil::safe_delete(m1); pdiutil::safe_delete(m2);
    pdiutil::safe_delete(h); pdiutil::safe_delete(t);
    return ok;
}

bool rsa_sign_pkcs1(const rsa_key *key, rsa_hash_alg alg,
                    const uint8_t *msg, size_t msg_len,
                    uint8_t *sig_out, size_t *sig_len) {
    if (!key->has_private) return false;
    size_t k = (size_t)bn_num_bytes(&key->n);
    if (k == 0 || k > RSA_MAX_KEY_BITS / 8) return false;

    uint8_t em[RSA_MAX_KEY_BITS / 8];
    if (!build_emsa_pkcs1(alg, msg, msg_len, em, k)) return false;

    bignum *m = pdiutil::safe_new<bignum>();
    bignum *s = pdiutil::safe_new<bignum>();
    if (!m || !s) { pdiutil::safe_delete(m); pdiutil::safe_delete(s); return false; }

    bool ok = bn_from_bytes(m, em, k) && rsa_private_op(key, m, s);
    if (ok) ok = bn_to_bytes(s, sig_out, k);
    if (ok && sig_len) *sig_len = k;

    pdiutil::safe_delete(m);
    pdiutil::safe_delete(s);
    return ok;
}

bool rsa_verify_pkcs1(const rsa_key *key, rsa_hash_alg alg,
                      const uint8_t *msg, size_t msg_len,
                      const uint8_t *sig, size_t sig_len) {
    size_t k = (size_t)bn_num_bytes(&key->n);
    if (k == 0 || k > RSA_MAX_KEY_BITS / 8) return false;
    if (sig_len != k) return false;

    uint8_t expected[RSA_MAX_KEY_BITS / 8];
    if (!build_emsa_pkcs1(alg, msg, msg_len, expected, k)) return false;

    bignum *s = pdiutil::safe_new<bignum>();
    bignum *m = pdiutil::safe_new<bignum>();
    if (!s || !m) { pdiutil::safe_delete(s); pdiutil::safe_delete(m); return false; }

    uint8_t em[RSA_MAX_KEY_BITS / 8];
    bool ok = bn_from_bytes(s, sig, sig_len) &&
              bn_modexp(m, s, &key->e, &key->n) &&
              bn_to_bytes(m, em, k);
    if (ok) ok = (memcmp(em, expected, k) == 0);

    pdiutil::safe_delete(s);
    pdiutil::safe_delete(m);
    return ok;
}
