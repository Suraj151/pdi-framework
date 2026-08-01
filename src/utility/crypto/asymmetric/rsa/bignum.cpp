/***************************** bignum util ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Aug 2026
******************************************************************************/

#include "bignum.h"
#include <utility/SafeAlloc.h>

static bn_yield_fn s_yield_hook = nullptr;

void bn_set_yield_hook(bn_yield_fn fn) { s_yield_hook = fn; }

static inline void s_yield() {
    if (s_yield_hook) s_yield_hook();
}

static inline uint32_t word_at(const bignum *a, int32_t i) {
    return (i < a->used) ? a->w[i] : 0u;
}

void bn_zero(bignum *a) {
    for (int32_t i = 0; i < BN_MAX_WORDS; i++) a->w[i] = 0;
    a->used = 0;
    // s_yield();
}

void bn_copy(bignum *r, const bignum *a) {
    for (int32_t i = 0; i < BN_MAX_WORDS; i++) r->w[i] = a->w[i];
    r->used = a->used;
    // s_yield();
}

void bn_set_u32(bignum *r, uint32_t v) {
    bn_zero(r);
    if (v) { r->w[0] = v; r->used = 1; }
}

void bn_norm(bignum *a) {
    int32_t i = BN_MAX_WORDS - 1;
    while (i >= 0 && a->w[i] == 0) i--;
    a->used = i + 1;
}

bool bn_is_zero(const bignum *a) { return a->used == 0; }

bool bn_is_odd(const bignum *a) { return a->used > 0 && (a->w[0] & 1u); }

int32_t bn_cmp(const bignum *a, const bignum *b) {
    if (a->used != b->used) return a->used > b->used ? 1 : -1;
    for (int32_t i = a->used - 1; i >= 0; i--) {
        if (a->w[i] != b->w[i]) return a->w[i] > b->w[i] ? 1 : -1;
    }
    return 0;
}

int32_t bn_bitlen(const bignum *a) {
    if (a->used == 0) return 0;
    uint32_t hi = a->w[a->used - 1];
    int32_t bits = (a->used - 1) * BN_WORD_BITS;
    while (hi) { bits++; hi >>= 1; }
    return bits;
}

bool bn_test_bit(const bignum *a, int32_t i) {
    if (i < 0) return false;
    int32_t wi = i / BN_WORD_BITS;
    if (wi >= a->used) return false;
    return (a->w[wi] >> (i % BN_WORD_BITS)) & 1u;
}

bool bn_from_bytes(bignum *r, const uint8_t *buf, size_t len) {
    bn_zero(r);
    // big-endian input -> little-endian limbs
    int32_t bit = 0;
    for (int32_t i = (int32_t)len - 1; i >= 0; i--) {
        int32_t wi = bit / BN_WORD_BITS;
        if (wi >= BN_MAX_WORDS) return false;
        r->w[wi] |= ((uint32_t)buf[i]) << (bit % BN_WORD_BITS);
        bit += 8;
    }
    bn_norm(r);
    return true;
}

int32_t bn_num_bytes(const bignum *a) {
    return (bn_bitlen(a) + 7) / 8;
}

bool bn_to_bytes(const bignum *a, uint8_t *buf, size_t len) {
    if ((size_t)bn_num_bytes(a) > len) return false;
    for (size_t i = 0; i < len; i++) {
        int32_t bit = (int32_t)(len - 1 - i) * 8;
        int32_t wi = bit / BN_WORD_BITS;
        uint32_t word = (wi < a->used) ? a->w[wi] : 0u;
        buf[i] = (uint8_t)(word >> (bit % BN_WORD_BITS));
    }
    return true;
}

bool bn_add(bignum *r, const bignum *a, const bignum *b) {
    int32_t n = a->used > b->used ? a->used : b->used;
    uint64_t carry = 0;
    for (int32_t i = 0; i < n; i++) {
        uint64_t s = (uint64_t)word_at(a, i) + word_at(b, i) + carry;
        if (i >= BN_MAX_WORDS) return false;
        r->w[i] = (uint32_t)s;
        carry = s >> 32;
    }
    if (carry) {
        if (n >= BN_MAX_WORDS) return false;
        r->w[n] = (uint32_t)carry;
        n++;
    }
    for (int32_t i = n; i < BN_MAX_WORDS; i++) r->w[i] = 0;
    r->used = n;
    bn_norm(r);
    return true;
}

// r = a - b, requires a >= b (magnitude)
bool bn_sub(bignum *r, const bignum *a, const bignum *b) {
    int64_t borrow = 0;
    int32_t n = a->used;
    for (int32_t i = 0; i < n; i++) {
        int64_t d = (int64_t)word_at(a, i) - word_at(b, i) - borrow;
        if (d < 0) { d += ((int64_t)1 << 32); borrow = 1; }
        else borrow = 0;
        r->w[i] = (uint32_t)d;
    }
    for (int32_t i = n; i < BN_MAX_WORDS; i++) r->w[i] = 0;
    r->used = n;
    bn_norm(r);
    return borrow == 0;
}

void bn_shl1(bignum *a) {
    uint32_t carry = 0;
    int32_t n = a->used;
    for (int32_t i = 0; i < n; i++) {
        uint32_t nc = a->w[i] >> 31;
        a->w[i] = (a->w[i] << 1) | carry;
        carry = nc;
    }
    if (carry && n < BN_MAX_WORDS) { a->w[n] = carry; a->used = n + 1; }
    else bn_norm(a);
}

void bn_shr1(bignum *a) {
    uint32_t carry = 0;
    for (int32_t i = a->used - 1; i >= 0; i--) {
        uint32_t nc = a->w[i] & 1u;
        a->w[i] = (a->w[i] >> 1) | (carry << 31);
        carry = nc;
    }
    bn_norm(a);
}

bool bn_mul(bignum *r, const bignum *a, const bignum *b) {
    if (a->used + b->used > BN_MAX_WORDS) return false;
    uint32_t *tmp = pdiutil::safe_new_array<uint32_t>(BN_MAX_WORDS);
    if (!tmp) return false;
    for (int32_t i = 0; i < BN_MAX_WORDS; i++) tmp[i] = 0;
    for (int32_t i = 0; i < a->used; i++) {
        uint64_t carry = 0;
        uint64_t ai = a->w[i];
        for (int32_t j = 0; j < b->used; j++) {
            uint64_t s = (uint64_t)tmp[i + j] + ai * b->w[j] + carry;
            tmp[i + j] = (uint32_t)s;
            carry = s >> 32;
        }
        tmp[i + b->used] += (uint32_t)carry;
        s_yield();
    }
    for (int32_t i = 0; i < BN_MAX_WORDS; i++) r->w[i] = tmp[i];
    bn_norm(r);
    pdiutil::safe_delete_array(tmp);
    return true;
}

// long division: a = q*b + rem. q and/or rem may be null.
bool bn_divmod(const bignum *a, const bignum *b, bignum *q, bignum *rem) {
    if (bn_is_zero(b)) return false;
    if (bn_cmp(a, b) < 0) {
        if (q) bn_zero(q);
        if (rem) bn_copy(rem, a);
        return true;
    }
    bignum *R = pdiutil::safe_new<bignum>();
    bignum *Q = pdiutil::safe_new<bignum>();
    if (!R || !Q) { pdiutil::safe_delete(R); pdiutil::safe_delete(Q); return false; }
    bn_zero(R);
    bn_zero(Q);
    int32_t n = bn_bitlen(a);
    for (int32_t i = n - 1; i >= 0; i--) {
        bn_shl1(R);
        if (bn_test_bit(a, i)) {
            R->w[0] |= 1u;
            if (R->used == 0) R->used = 1;
        }
        if (bn_cmp(R, b) >= 0) {
            bn_sub(R, R, b);
            int32_t wi = i / BN_WORD_BITS;
            Q->w[wi] |= (1u << (i % BN_WORD_BITS));
            if (Q->used < wi + 1) Q->used = wi + 1;
        }
        if ((i & 0x3F) == 0) s_yield();
    }
    bn_norm(Q);
    bn_norm(R);
    if (q) bn_copy(q, Q);
    if (rem) bn_copy(rem, R);
    pdiutil::safe_delete(R);
    pdiutil::safe_delete(Q);
    return true;
}

bool bn_mod(bignum *r, const bignum *a, const bignum *m) {
    return bn_divmod(a, m, nullptr, r);
}

bool bn_mulmod(bignum *r, const bignum *a, const bignum *b, const bignum *m) {
    bignum *t = pdiutil::safe_new<bignum>();
    if (!t) return false;
    bool ok = bn_mul(t, a, b) && bn_mod(r, t, m);
    pdiutil::safe_delete(t);
    return ok;
}

// -m^{-1} mod 2^32 (Montgomery). m0 must be odd.
static uint32_t mont_n0inv(uint32_t m0) {
    uint32_t inv = 1;
    for (int32_t i = 0; i < 5; i++) inv = inv * (2u - m0 * inv);
    return (uint32_t)(0u - inv);
}

// CIOS Montgomery multiply: r = a*b*R^{-1} mod m, with R = 2^(32*k).
// Requires m odd, a < m, b < m, k = m->used.
static void bn_montmul(bignum *r, const bignum *a, const bignum *b,
                       const bignum *m, uint32_t n0inv, int32_t k) {
    uint32_t *t = pdiutil::safe_new_array<uint32_t>(BN_MAX_WORDS + 2);
    if (!t) { bn_zero(r); return; }
    for (int32_t i = 0; i < k + 2; i++) t[i] = 0;

    for (int32_t i = 0; i < k; i++) {
        uint64_t bi = word_at(b, i);
        uint64_t C = 0;
        for (int32_t j = 0; j < k; j++) {
            uint64_t s = (uint64_t)t[j] + (uint64_t)word_at(a, j) * bi + C;
            t[j] = (uint32_t)s;
            C = s >> 32;
        }
        uint64_t s = (uint64_t)t[k] + C;
        t[k] = (uint32_t)s;
        t[k + 1] = (uint32_t)(s >> 32);

        uint32_t mm = (uint32_t)((uint64_t)t[0] * n0inv);
        uint64_t s0 = (uint64_t)t[0] + (uint64_t)mm * word_at(m, 0);
        C = s0 >> 32; // low word is zero by construction
        for (int32_t j = 1; j < k; j++) {
            uint64_t sj = (uint64_t)t[j] + (uint64_t)mm * word_at(m, j) + C;
            t[j - 1] = (uint32_t)sj;
            C = sj >> 32;
        }
        uint64_t sk = (uint64_t)t[k] + C;
        t[k - 1] = (uint32_t)sk;
        C = sk >> 32;
        t[k] = (uint32_t)((uint64_t)t[k + 1] + C);
        t[k + 1] = 0;
    }

    bn_zero(r);
    for (int32_t i = 0; i <= k && i < BN_MAX_WORDS; i++) r->w[i] = t[i];
    r->used = k + 1;
    bn_norm(r);
    if (bn_cmp(r, m) >= 0) bn_sub(r, r, m);
    pdiutil::safe_delete_array(t);
}

// r = base^exp mod m. m must be odd (RSA moduli/primes are).
bool bn_modexp(bignum *r, const bignum *base, const bignum *exp, const bignum *m) {
    if (bn_is_zero(m) || !(m->w[0] & 1u)) return false;
    bignum *one = pdiutil::safe_new<bignum>();
    if (!one) return false;
    bn_set_u32(one, 1);
    if (bn_cmp(m, one) == 0) { bn_zero(r); pdiutil::safe_delete(one); return true; }
    if (bn_is_zero(exp)) { bn_copy(r, one); pdiutil::safe_delete(one); return true; }

    int32_t k = m->used;
    uint32_t n0inv = mont_n0inv(m->w[0]);

    bignum *rr = pdiutil::safe_new<bignum>();
    bignum *base_red = pdiutil::safe_new<bignum>();
    bignum *base_mont = pdiutil::safe_new<bignum>();
    bignum *result = pdiutil::safe_new<bignum>();
    bignum *tmp = pdiutil::safe_new<bignum>();
    if (!rr || !base_red || !base_mont || !result || !tmp) {
        pdiutil::safe_delete(one);
        pdiutil::safe_delete(rr); pdiutil::safe_delete(base_red);
        pdiutil::safe_delete(base_mont); pdiutil::safe_delete(result);
        pdiutil::safe_delete(tmp);
        return false;
    }

    // RR = R^2 mod m = 2^(64k) mod m
    bn_set_u32(rr, 1);
    for (int32_t i = 0; i < 64 * k; i++) {
        bn_shl1(rr);
        if (bn_cmp(rr, m) >= 0) bn_sub(rr, rr, m);
        if ((i & 0x7F) == 0) s_yield();
    }

    bn_mod(base_red, base, m);
    bn_montmul(base_mont, base_red, rr, m, n0inv, k);   // base * R mod m
    bn_montmul(result, one, rr, m, n0inv, k);            // R mod m (Montgomery 1)

    for (int32_t i = bn_bitlen(exp) - 1; i >= 0; i--) {
        bn_montmul(tmp, result, result, m, n0inv, k);
        bn_copy(result, tmp);
        if (bn_test_bit(exp, i)) {
            bn_montmul(tmp, result, base_mont, m, n0inv, k);
            bn_copy(result, tmp);
        }
        s_yield();
    }

    bn_montmul(r, result, one, m, n0inv, k);             // back from Montgomery

    pdiutil::safe_delete(one);
    pdiutil::safe_delete(rr); pdiutil::safe_delete(base_red);
    pdiutil::safe_delete(base_mont); pdiutil::safe_delete(result);
    pdiutil::safe_delete(tmp);
    return true;
}

bool bn_gcd(bignum *r, const bignum *a, const bignum *b) {
    bignum *x = pdiutil::safe_new<bignum>();
    bignum *y = pdiutil::safe_new<bignum>();
    bignum *t = pdiutil::safe_new<bignum>();
    if (!x || !y || !t) { pdiutil::safe_delete(x); pdiutil::safe_delete(y); pdiutil::safe_delete(t); return false; }
    bn_copy(x, a);
    bn_copy(y, b);
    while (!bn_is_zero(y)) {
        if (!bn_mod(t, x, y)) { pdiutil::safe_delete(x); pdiutil::safe_delete(y); pdiutil::safe_delete(t); return false; }
        bn_copy(x, y);
        bn_copy(y, t);
        s_yield();
    }
    bn_copy(r, x);
    pdiutil::safe_delete(x); pdiutil::safe_delete(y); pdiutil::safe_delete(t);
    return true;
}

// r = a^{-1} mod m via extended Euclid, coefficients kept in [0, m).
bool bn_modinv(bignum *r, const bignum *a, const bignum *m) {
    bignum *r0 = pdiutil::safe_new<bignum>();
    bignum *r1 = pdiutil::safe_new<bignum>();
    bignum *t0 = pdiutil::safe_new<bignum>();
    bignum *t1 = pdiutil::safe_new<bignum>();
    bignum *q = pdiutil::safe_new<bignum>();
    bignum *rem = pdiutil::safe_new<bignum>();
    bignum *qt = pdiutil::safe_new<bignum>();
    bignum *newt = pdiutil::safe_new<bignum>();
    if (!r0 || !r1 || !t0 || !t1 || !q || !rem || !qt || !newt) {
        pdiutil::safe_delete(r0); pdiutil::safe_delete(r1);
        pdiutil::safe_delete(t0); pdiutil::safe_delete(t1);
        pdiutil::safe_delete(q); pdiutil::safe_delete(rem);
        pdiutil::safe_delete(qt); pdiutil::safe_delete(newt);
        return false;
    }

    bool ok = true;
    bn_copy(r0, m);
    ok = bn_mod(r1, a, m);
    bn_set_u32(t0, 0);
    bn_set_u32(t1, 1);

    while (ok && !bn_is_zero(r1)) {
        if (!bn_divmod(r0, r1, q, rem)) { ok = false; break; }
        // (r0, r1) = (r1, rem)
        bn_copy(r0, r1);
        bn_copy(r1, rem);
        // newt = (t0 - (q*t1 mod m)) mod m
        if (!bn_mulmod(qt, q, t1, m)) { ok = false; break; }
        if (bn_cmp(t0, qt) >= 0) {
            bn_sub(newt, t0, qt);
        } else {
            bn_add(newt, t0, m);
            bn_sub(newt, newt, qt);
        }
        bn_copy(t0, t1);
        bn_copy(t1, newt);
        s_yield();
    }

    // inverse exists only when gcd == 1
    bignum one;
    bn_set_u32(&one, 1);
    if (ok && bn_cmp(r0, &one) == 0) {
        bn_copy(r, t0);
    } else {
        ok = false;
    }

    pdiutil::safe_delete(r0); pdiutil::safe_delete(r1);
    pdiutil::safe_delete(t0); pdiutil::safe_delete(t1);
    pdiutil::safe_delete(q); pdiutil::safe_delete(rem);
    pdiutil::safe_delete(qt); pdiutil::safe_delete(newt);
    return ok;
}

static uint32_t bn_mod_u32(const bignum *a, uint32_t m) {
    uint64_t rem = 0;
    for (int32_t i = a->used - 1; i >= 0; i--) {
        rem = ((rem << 32) | a->w[i]) % m;
    }
    return (uint32_t)rem;
}

// exact bit-length random, top bit forced set
bool bn_rand_bits(bignum *r, int32_t bits, bn_rng_fn rng) {
    if (bits <= 0 || bits > RSA_MAX_KEY_BITS) return false;
    int32_t nbytes = (bits + 7) / 8;
    uint8_t *buf = pdiutil::safe_new_array<uint8_t>(nbytes);
    if (!buf) return false;
    rng(buf, (size_t)nbytes);
    bool ok = bn_from_bytes(r, buf, (size_t)nbytes);
    pdiutil::safe_delete_array(buf);
    if (!ok) return false;
    // clear bits at or above `bits`
    for (int32_t i = bits; i < nbytes * 8; i++) {
        int32_t wi = i / BN_WORD_BITS;
        if (wi < BN_MAX_WORDS) r->w[wi] &= ~(1u << (i % BN_WORD_BITS));
    }
    // force top bit set for exact length
    int32_t wi = (bits - 1) / BN_WORD_BITS;
    r->w[wi] |= (1u << ((bits - 1) % BN_WORD_BITS));
    if (r->used < wi + 1) r->used = wi + 1;
    bn_norm(r);
    return true;
}

// first primes for cheap trial division prefilter
static const uint16_t s_small_primes[] = {
    3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
    73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151,
    157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233,
    239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317,
    331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419,
    421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503,
    509, 521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607,
    613, 617, 619, 631, 641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701,
    709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787, 797, 809, 811,
    821, 823, 827, 829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911,
    919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997
};

bool bn_is_probable_prime(const bignum *a, int32_t rounds, bn_rng_fn rng) {
    if (a->used == 0) return false;
    bignum *two = pdiutil::safe_new<bignum>();
    if (!two) return false;
    bn_set_u32(two, 2);
    if (bn_cmp(a, two) < 0) { pdiutil::safe_delete(two); return false; }
    if (!(a->w[0] & 1u)) { bool eq = bn_cmp(a, two) == 0; pdiutil::safe_delete(two); return eq; }
    pdiutil::safe_delete(two);

    for (size_t i = 0; i < sizeof(s_small_primes) / sizeof(s_small_primes[0]); i++) {
        uint32_t p = s_small_primes[i];
        uint32_t rem = bn_mod_u32(a, p);
        if (rem == 0) {
            // divisible by small prime => prime only if equal to it
            bignum *bp = pdiutil::safe_new<bignum>();
            if (!bp) return false;
            bn_set_u32(bp, p);
            bool eq = bn_cmp(a, bp) == 0;
            pdiutil::safe_delete(bp);
            return eq;
        }
        // if ((i & 0x1F) == 0) s_yield();
    }

    bignum *n1 = pdiutil::safe_new<bignum>();   // a - 1
    bignum *d = pdiutil::safe_new<bignum>();    // odd part of a-1
    bignum *x = pdiutil::safe_new<bignum>();
    bignum *base = pdiutil::safe_new<bignum>();
    bignum *three = pdiutil::safe_new<bignum>();
    bignum *one = pdiutil::safe_new<bignum>();
    bignum *sq = pdiutil::safe_new<bignum>();
    if (!n1 || !d || !x || !base || !three || !one || !sq) {
        pdiutil::safe_delete(n1); pdiutil::safe_delete(d);
        pdiutil::safe_delete(x); pdiutil::safe_delete(base);
        pdiutil::safe_delete(three); pdiutil::safe_delete(one);
        pdiutil::safe_delete(sq);
        return false;
    }

    bn_set_u32(one, 1);
    bn_sub(n1, a, one); // n1 = a - 1
    bn_copy(d, n1);
    int32_t s = 0;
    while (!(d->w[0] & 1u)) { bn_shr1(d); s++; }

    bool prime = true;
    for (int32_t round = 0; round < rounds && prime; round++) {
        // choose a base in [2, a-2]
        bn_rand_bits(base, bn_bitlen(a), rng);
        bn_set_u32(three, 3);
        bignum *am2 = pdiutil::safe_new<bignum>();
        if (!am2) { prime = false; break; }
        bn_sub(am2, a, three); // a - 3
        if (bn_is_zero(am2) || bn_cmp(am2, one) < 0) { bn_set_u32(base, 2); }
        else {
            bn_mod(base, base, am2); // base in [0, a-4]
            bn_add(base, base, one); // +1 -> [1, a-3]
            bn_add(base, base, one); // +1 -> [2, a-2]
        }
        pdiutil::safe_delete(am2);

        if (!bn_modexp(x, base, d, a)) { prime = false; break; }
        if (bn_cmp(x, one) == 0 || bn_cmp(x, n1) == 0) continue;

        bool witness = true;
        for (int32_t rr = 0; rr < s - 1; rr++) {
            bn_copy(sq, x);
            if (!bn_mulmod(x, sq, sq, a)) { witness = true; break; }
            if (bn_cmp(x, n1) == 0) { witness = false; break; }
            s_yield();
        }
        if (witness) prime = false;
        // restore x=1 for reuse of the [2,a-2] branch is not needed
        bn_set_u32(x, 1);
        s_yield();
    }

    pdiutil::safe_delete(n1); pdiutil::safe_delete(d);
    pdiutil::safe_delete(x); pdiutil::safe_delete(base);
    pdiutil::safe_delete(three); pdiutil::safe_delete(one);
    pdiutil::safe_delete(sq);
    return prime;
}

bool bn_gen_prime(bignum *r, int32_t bits, bn_rng_fn rng) {
    bignum *two = pdiutil::safe_new<bignum>();
    if (!two) return false;
    bn_set_u32(two, 2);
    bool found = false;
    for (int32_t attempts = 0; attempts < 100000 && !found; attempts++) {
        if (!bn_rand_bits(r, bits, rng)) break;
        r->w[0] |= 1u; // force odd
        // force top two bits so products keep full size
        int32_t top = bits - 1;
        r->w[top / BN_WORD_BITS] |= (1u << (top % BN_WORD_BITS));
        if (bits >= 2) {
            int32_t t2 = bits - 2;
            r->w[t2 / BN_WORD_BITS] |= (1u << (t2 % BN_WORD_BITS));
        }
        bn_norm(r);
        // scan odd candidates upward
        for (int32_t step = 0; step < 4096; step++) {
            if (bn_bitlen(r) != bits) break; // overflowed target size, regenerate
            if (bn_is_probable_prime(r, 5, rng)) { found = true; break; }
            bn_add(r, r, two);
            s_yield();
        }
    }
    pdiutil::safe_delete(two);
    return found;
}
