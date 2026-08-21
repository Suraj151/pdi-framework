/********************************* RSA Tests **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Key generation at realistic sizes is slow, so the suite generates one 512 bit
key and shares it. Size only affects the modulus, not the PKCS#1 path.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <utility/crypto/asymmetric/rsa/rsa.h>

static void testRng(uint8_t *buf, size_t len)
{
    static uint32_t state = 0xC0FFEE11u;
    for (size_t i = 0; i < len; i++)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        buf[i] = (uint8_t)(state & 0xFFu);
    }
}

/**
 * One shared key, built on first use, so each test does not pay for keygen.
 */
static rsa_key *sharedKey()
{
    static rsa_key key;
    static bool ready = false;

    if (!ready)
    {
        rsa_key_init(&key);
        ready = rsa_generate_keypair(&key, 512, testRng);
    }

    return ready ? &key : nullptr;
}

/**
 * A wider key for SHA-512, whose DigestInfo plus PKCS#1 padding does not fit
 * inside a 512 bit modulus.
 */
static rsa_key *wideKey()
{
    static rsa_key key;
    static bool ready = false;
    static bool attempted = false;

    if (!attempted)
    {
        attempted = true;
        rsa_key_init(&key);
        ready = rsa_generate_keypair(&key, 1024, testRng);
    }

    return ready ? &key : nullptr;
}

TEST(rsa, generates_a_key_with_private_and_crt_parts)
{
    rsa_key *key = sharedKey();
    ASSERT_NOT_NULL(key);
    ASSERT_TRUE(key->has_private);
    ASSERT_TRUE(key->has_crt);
}

TEST(rsa, the_modulus_has_the_requested_width)
{
    rsa_key *key = sharedKey();
    ASSERT_NOT_NULL(key);
    ASSERT_EQ(bn_bitlen(&key->n), 512);
}

TEST(rsa, the_public_exponent_is_the_expected_value)
{
    rsa_key *key = sharedKey();
    ASSERT_NOT_NULL(key);

    bignum expected;
    bn_set_u32(&expected, RSA_PUBLIC_EXPONENT);
    ASSERT_EQ(bn_cmp(&key->e, &expected), 0);
}

TEST(rsa, the_primes_multiply_back_to_the_modulus)
{
    rsa_key *key = sharedKey();
    ASSERT_NOT_NULL(key);

    bignum product;
    ASSERT_TRUE(bn_mul(&product, &key->p, &key->q));
    ASSERT_EQ(bn_cmp(&product, &key->n), 0);
}

TEST(rsa, sha256_signature_verifies)
{
    rsa_key *key = sharedKey();
    ASSERT_NOT_NULL(key);

    const char *message = "pdi framework host key signature";
    uint8_t sig[256];
    size_t siglen = 0;

    ASSERT_TRUE(rsa_sign_pkcs1(key, RSA_HASH_SHA256, (const uint8_t *)message,
                               strlen(message), sig, &siglen));
    ASSERT_EQ(siglen, (size_t)64);
    ASSERT_TRUE(rsa_verify_pkcs1(key, RSA_HASH_SHA256, (const uint8_t *)message,
                                 strlen(message), sig, siglen));
}

TEST(rsa, sha512_is_refused_when_the_modulus_is_too_narrow)
{
    rsa_key *key = sharedKey();
    ASSERT_NOT_NULL(key);

    const char *message = "too wide for a 512 bit modulus";
    uint8_t sig[256];
    size_t siglen = 0;

    ASSERT_FALSE(rsa_sign_pkcs1(key, RSA_HASH_SHA512, (const uint8_t *)message,
                                strlen(message), sig, &siglen));
}

TEST(rsa, sha512_signature_verifies_on_a_wider_key)
{
    rsa_key *key = wideKey();
    ASSERT_NOT_NULL(key);

    const char *message = "second digest algorithm";
    uint8_t sig[256];
    size_t siglen = 0;

    ASSERT_TRUE(rsa_sign_pkcs1(key, RSA_HASH_SHA512, (const uint8_t *)message,
                               strlen(message), sig, &siglen));
    ASSERT_EQ(siglen, (size_t)128);
    ASSERT_TRUE(rsa_verify_pkcs1(key, RSA_HASH_SHA512, (const uint8_t *)message,
                                 strlen(message), sig, siglen));
}

TEST(rsa, a_wider_key_round_trips_sha256_too)
{
    rsa_key *key = wideKey();
    ASSERT_NOT_NULL(key);

    const char *message = "1024 bit modulus";
    uint8_t sig[256];
    size_t siglen = 0;

    ASSERT_TRUE(rsa_sign_pkcs1(key, RSA_HASH_SHA256, (const uint8_t *)message,
                               strlen(message), sig, &siglen));
    ASSERT_TRUE(rsa_verify_pkcs1(key, RSA_HASH_SHA256, (const uint8_t *)message,
                                 strlen(message), sig, siglen));
}

TEST(rsa, signing_is_deterministic)
{
    rsa_key *key = sharedKey();
    ASSERT_NOT_NULL(key);

    const char *message = "same message twice";
    uint8_t first[256];
    uint8_t second[256];
    size_t firstlen = 0;
    size_t secondlen = 0;

    ASSERT_TRUE(rsa_sign_pkcs1(key, RSA_HASH_SHA256, (const uint8_t *)message,
                               strlen(message), first, &firstlen));
    ASSERT_TRUE(rsa_sign_pkcs1(key, RSA_HASH_SHA256, (const uint8_t *)message,
                               strlen(message), second, &secondlen));

    ASSERT_EQ(firstlen, secondlen);
    ASSERT_MEMEQ(first, second, firstlen);
}

TEST(rsa, rejects_a_tampered_signature)
{
    rsa_key *key = sharedKey();
    ASSERT_NOT_NULL(key);

    const char *message = "authentic";
    uint8_t sig[256];
    size_t siglen = 0;

    ASSERT_TRUE(rsa_sign_pkcs1(key, RSA_HASH_SHA256, (const uint8_t *)message,
                               strlen(message), sig, &siglen));
    sig[siglen - 1] ^= 0x01;

    ASSERT_FALSE(rsa_verify_pkcs1(key, RSA_HASH_SHA256, (const uint8_t *)message,
                                  strlen(message), sig, siglen));
}

TEST(rsa, rejects_a_tampered_message)
{
    rsa_key *key = sharedKey();
    ASSERT_NOT_NULL(key);

    char message[32];
    uint8_t sig[256];
    size_t siglen = 0;

    strcpy(message, "authentic");
    ASSERT_TRUE(rsa_sign_pkcs1(key, RSA_HASH_SHA256, (const uint8_t *)message,
                               strlen(message), sig, &siglen));

    message[0] = 'A';
    ASSERT_FALSE(rsa_verify_pkcs1(key, RSA_HASH_SHA256, (const uint8_t *)message,
                                  strlen(message), sig, siglen));
}

TEST(rsa, rejects_a_signature_verified_under_the_wrong_digest)
{
    rsa_key *key = sharedKey();
    ASSERT_NOT_NULL(key);

    const char *message = "digest mismatch";
    uint8_t sig[256];
    size_t siglen = 0;

    ASSERT_TRUE(rsa_sign_pkcs1(key, RSA_HASH_SHA256, (const uint8_t *)message,
                               strlen(message), sig, &siglen));
    ASSERT_FALSE(rsa_verify_pkcs1(key, RSA_HASH_SHA512, (const uint8_t *)message,
                                  strlen(message), sig, siglen));
}

TEST(rsa, rejects_a_truncated_signature)
{
    rsa_key *key = sharedKey();
    ASSERT_NOT_NULL(key);

    const char *message = "length matters";
    uint8_t sig[256];
    size_t siglen = 0;

    ASSERT_TRUE(rsa_sign_pkcs1(key, RSA_HASH_SHA256, (const uint8_t *)message,
                               strlen(message), sig, &siglen));
    ASSERT_FALSE(rsa_verify_pkcs1(key, RSA_HASH_SHA256, (const uint8_t *)message,
                                  strlen(message), sig, siglen - 1));
}

TEST(rsa, signs_an_empty_message)
{
    rsa_key *key = sharedKey();
    ASSERT_NOT_NULL(key);

    uint8_t sig[256];
    size_t siglen = 0;

    ASSERT_TRUE(rsa_sign_pkcs1(key, RSA_HASH_SHA256, nullptr, 0, sig, &siglen));
    ASSERT_TRUE(rsa_verify_pkcs1(key, RSA_HASH_SHA256, nullptr, 0, sig, siglen));
}

TEST(rsa, a_public_only_key_still_verifies)
{
    rsa_key *key = sharedKey();
    ASSERT_NOT_NULL(key);

    const char *message = "verify with the public half";
    uint8_t sig[256];
    size_t siglen = 0;

    ASSERT_TRUE(rsa_sign_pkcs1(key, RSA_HASH_SHA256, (const uint8_t *)message,
                               strlen(message), sig, &siglen));

    rsa_key publiconly;
    rsa_key_init(&publiconly);
    bn_copy(&publiconly.n, &key->n);
    bn_copy(&publiconly.e, &key->e);
    publiconly.has_private = false;
    publiconly.has_crt = false;

    ASSERT_TRUE(rsa_verify_pkcs1(&publiconly, RSA_HASH_SHA256, (const uint8_t *)message,
                                 strlen(message), sig, siglen));
}

TEST(rsa, a_public_only_key_cannot_sign)
{
    rsa_key *key = sharedKey();
    ASSERT_NOT_NULL(key);

    rsa_key publiconly;
    rsa_key_init(&publiconly);
    bn_copy(&publiconly.n, &key->n);
    bn_copy(&publiconly.e, &key->e);
    publiconly.has_private = false;
    publiconly.has_crt = false;

    uint8_t sig[256];
    size_t siglen = 0;
    ASSERT_FALSE(rsa_sign_pkcs1(&publiconly, RSA_HASH_SHA256, (const uint8_t *)"x", 1,
                                sig, &siglen));
}
