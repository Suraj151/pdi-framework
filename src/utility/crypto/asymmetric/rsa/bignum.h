/***************************** bignum util ***********************************
This file is part of the pdi stack.

Minimal portable big-integer arithmetic for RSA. Fixed-capacity, little-endian
32-bit limbs. Montgomery reduction powers the modexp hot path; bit-serial long
division backs the infrequent setup paths (divmod, gcd, modular inverse).

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Aug 2026
******************************************************************************/

#ifndef _CRYPTO_BIGNUM_H_
#define _CRYPTO_BIGNUM_H_

#include <stddef.h>
#include <utility/crypto/fixedint.h>

#ifndef RSA_MAX_KEY_BITS
#define RSA_MAX_KEY_BITS 2048
#endif

#define BN_WORD_BITS 32
#define BN_MAX_WORDS (((RSA_MAX_KEY_BITS) / BN_WORD_BITS) * 2 + 4)

// Random byte source supplied by the caller (device-agnostic).
typedef void (*bn_rng_fn)(uint8_t *buf, size_t len);

// Optional cooperative yield, invoked inside long inner loops so the caller can
// service the watchdog. Device-agnostic: pass nullptr to disable.
typedef void (*bn_yield_fn)(void);
void bn_set_yield_hook(bn_yield_fn fn);

struct bignum {
    uint32_t w[BN_MAX_WORDS];
    int32_t used; // count of significant limbs (0 => value is zero)
};

void bn_zero(bignum *a);
void bn_copy(bignum *r, const bignum *a);
void bn_set_u32(bignum *r, uint32_t v);
void bn_norm(bignum *a);

bool bn_is_zero(const bignum *a);
bool bn_is_odd(const bignum *a);
int32_t bn_cmp(const bignum *a, const bignum *b);
int32_t bn_bitlen(const bignum *a);
bool bn_test_bit(const bignum *a, int32_t i);

bool bn_from_bytes(bignum *r, const uint8_t *buf, size_t len);
int32_t bn_num_bytes(const bignum *a);
bool bn_to_bytes(const bignum *a, uint8_t *buf, size_t len);

bool bn_add(bignum *r, const bignum *a, const bignum *b);
bool bn_sub(bignum *r, const bignum *a, const bignum *b);
void bn_shl1(bignum *a);
void bn_shr1(bignum *a);

bool bn_mul(bignum *r, const bignum *a, const bignum *b);
bool bn_divmod(const bignum *a, const bignum *b, bignum *q, bignum *rem);
bool bn_mod(bignum *r, const bignum *a, const bignum *m);
bool bn_mulmod(bignum *r, const bignum *a, const bignum *b, const bignum *m);
bool bn_modexp(bignum *r, const bignum *base, const bignum *exp, const bignum *m);

bool bn_gcd(bignum *r, const bignum *a, const bignum *b);
bool bn_modinv(bignum *r, const bignum *a, const bignum *m);

bool bn_rand_bits(bignum *r, int32_t bits, bn_rng_fn rng);
bool bn_is_probable_prime(const bignum *a, int32_t rounds, bn_rng_fn rng);
bool bn_gen_prime(bignum *r, int32_t bits, bn_rng_fn rng);

#endif // _CRYPTO_BIGNUM_H_
