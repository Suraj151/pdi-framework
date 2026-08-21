/******************************* Bignum Tests *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <utility/DataTypeConversions.h>
#include <utility/crypto/asymmetric/rsa/bignum.h>

/**
 * A fixed sequence stands in for entropy so a run reproduces exactly. The
 * primality test needs a real generator for its Miller-Rabin bases.
 */
static void testRng(uint8_t *buf, size_t len)
{
    static uint32_t state = 0x13579BDFu;
    for (size_t i = 0; i < len; i++)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        buf[i] = (uint8_t)(state & 0xFFu);
    }
}

/**
 * Load a bignum from a hex string, most significant byte first.
 */
static bool fromHex(bignum *out, const char *hex)
{
    uint8_t bytes[BN_MAX_WORDS * 4];
    size_t bytelen = strlen(hex) / 2;
    if (bytelen > sizeof(bytes))
    {
        return false;
    }
    if (!HexStringToBytes(hex, (uint8_t)bytelen, bytes))
    {
        return false;
    }
    return bn_from_bytes(out, bytes, bytelen);
}

/**
 * Render a bignum as hex without leading zero bytes.
 */
static void toHex(const bignum *value, char *out, size_t outsize)
{
    uint8_t bytes[BN_MAX_WORDS * 4];
    int32_t len = bn_num_bytes(value);

    if (len <= 0)
    {
        strcpy(out, "00");
        return;
    }

    bn_to_bytes(value, bytes, (size_t)len);
    BytesToHexString(bytes, (uint8_t)len, out);
}

TEST(bignum, set_and_compare_small_values)
{
    bignum a;
    bignum b;
    bn_set_u32(&a, 42);
    bn_set_u32(&b, 42);

    ASSERT_EQ(bn_cmp(&a, &b), 0);

    bn_set_u32(&b, 43);
    ASSERT_LT(bn_cmp(&a, &b), 0);
    ASSERT_GT(bn_cmp(&b, &a), 0);
}

TEST(bignum, zero_is_reported_as_zero)
{
    bignum a;
    bn_zero(&a);
    ASSERT_TRUE(bn_is_zero(&a));

    bn_set_u32(&a, 1);
    ASSERT_FALSE(bn_is_zero(&a));
}

TEST(bignum, odd_detection)
{
    bignum a;
    bn_set_u32(&a, 7);
    ASSERT_TRUE(bn_is_odd(&a));

    bn_set_u32(&a, 8);
    ASSERT_FALSE(bn_is_odd(&a));
}

TEST(bignum, bit_length_and_bit_test)
{
    bignum a;
    bn_set_u32(&a, 0x80u);

    ASSERT_EQ(bn_bitlen(&a), 8);
    ASSERT_TRUE(bn_test_bit(&a, 7));
    ASSERT_FALSE(bn_test_bit(&a, 6));
}

TEST(bignum, copy_produces_an_equal_value)
{
    bignum a;
    bignum b;
    ASSERT_TRUE(fromHex(&a, "0123456789abcdef0123456789abcdef"));
    bn_copy(&b, &a);
    ASSERT_EQ(bn_cmp(&a, &b), 0);
}

TEST(bignum, byte_round_trip)
{
    bignum a;
    char hex[80];

    ASSERT_TRUE(fromHex(&a, "deadbeefcafebabe0102030405060708"));
    toHex(&a, hex, sizeof(hex));
    ASSERT_STREQ(hex, "deadbeefcafebabe0102030405060708");
}

TEST(bignum, addition_carries_across_words)
{
    bignum a;
    bignum b;
    bignum r;
    char hex[80];

    ASSERT_TRUE(fromHex(&a, "ffffffffffffffff"));
    bn_set_u32(&b, 1);
    ASSERT_TRUE(bn_add(&r, &a, &b));

    toHex(&r, hex, sizeof(hex));
    ASSERT_STREQ(hex, "010000000000000000");
}

TEST(bignum, subtraction_borrows_across_words)
{
    bignum a;
    bignum b;
    bignum r;
    char hex[80];

    ASSERT_TRUE(fromHex(&a, "010000000000000000"));
    bn_set_u32(&b, 1);
    ASSERT_TRUE(bn_sub(&r, &a, &b));

    toHex(&r, hex, sizeof(hex));
    ASSERT_STREQ(hex, "ffffffffffffffff");
}

TEST(bignum, add_then_subtract_returns_the_original)
{
    bignum a;
    bignum b;
    bignum sum;
    bignum back;

    ASSERT_TRUE(fromHex(&a, "0123456789abcdef0123456789abcdef"));
    ASSERT_TRUE(fromHex(&b, "fedcba9876543210"));

    ASSERT_TRUE(bn_add(&sum, &a, &b));
    ASSERT_TRUE(bn_sub(&back, &sum, &b));
    ASSERT_EQ(bn_cmp(&back, &a), 0);
}

TEST(bignum, shift_left_and_right_are_inverse)
{
    bignum a;
    bignum original;

    ASSERT_TRUE(fromHex(&a, "0123456789abcdef"));
    bn_copy(&original, &a);

    bn_shl1(&a);
    bn_shr1(&a);
    ASSERT_EQ(bn_cmp(&a, &original), 0);
}

TEST(bignum, shift_left_doubles)
{
    bignum a;
    bignum expected;

    bn_set_u32(&a, 0x1234);
    bn_set_u32(&expected, 0x2468);
    bn_shl1(&a);

    ASSERT_EQ(bn_cmp(&a, &expected), 0);
}

TEST(bignum, multiplication_matches_a_known_product)
{
    bignum a;
    bignum b;
    bignum r;
    char hex[80];

    ASSERT_TRUE(fromHex(&a, "ffffffff"));
    ASSERT_TRUE(fromHex(&b, "ffffffff"));
    ASSERT_TRUE(bn_mul(&r, &a, &b));

    toHex(&r, hex, sizeof(hex));
    ASSERT_STREQ(hex, "fffffffe00000001");
}

TEST(bignum, multiply_by_zero_is_zero)
{
    bignum a;
    bignum zero;
    bignum r;

    ASSERT_TRUE(fromHex(&a, "0123456789abcdef"));
    bn_zero(&zero);
    ASSERT_TRUE(bn_mul(&r, &a, &zero));
    ASSERT_TRUE(bn_is_zero(&r));
}

TEST(bignum, divmod_reconstructs_the_dividend)
{
    bignum a;
    bignum b;
    bignum q;
    bignum rem;
    bignum product;
    bignum back;

    ASSERT_TRUE(fromHex(&a, "0123456789abcdef0123456789abcdef"));
    ASSERT_TRUE(fromHex(&b, "fedcba98"));

    ASSERT_TRUE(bn_divmod(&a, &b, &q, &rem));
    ASSERT_TRUE(bn_mul(&product, &q, &b));
    ASSERT_TRUE(bn_add(&back, &product, &rem));

    ASSERT_EQ(bn_cmp(&back, &a), 0);
    ASSERT_LT(bn_cmp(&rem, &b), 0);
}

TEST(bignum, mod_of_a_multiple_is_zero)
{
    bignum m;
    bignum k;
    bignum product;
    bignum r;

    ASSERT_TRUE(fromHex(&m, "fedcba9876543211"));
    bn_set_u32(&k, 7);
    ASSERT_TRUE(bn_mul(&product, &m, &k));
    ASSERT_TRUE(bn_mod(&r, &product, &m));

    ASSERT_TRUE(bn_is_zero(&r));
}

TEST(bignum, mulmod_matches_multiply_then_reduce)
{
    bignum a;
    bignum b;
    bignum m;
    bignum viaMulmod;
    bignum product;
    bignum viaReduce;

    ASSERT_TRUE(fromHex(&a, "0123456789abcdef"));
    ASSERT_TRUE(fromHex(&b, "fedcba9876543210"));
    ASSERT_TRUE(fromHex(&m, "f00000000000000f"));

    ASSERT_TRUE(bn_mulmod(&viaMulmod, &a, &b, &m));
    ASSERT_TRUE(bn_mul(&product, &a, &b));
    ASSERT_TRUE(bn_mod(&viaReduce, &product, &m));

    ASSERT_EQ(bn_cmp(&viaMulmod, &viaReduce), 0);
}

TEST(bignum, modexp_matches_a_small_known_result)
{
    bignum base;
    bignum exponent;
    bignum modulus;
    bignum r;
    bignum expected;

    // 7^11 mod 13 == 2
    bn_set_u32(&base, 7);
    bn_set_u32(&exponent, 11);
    bn_set_u32(&modulus, 13);
    bn_set_u32(&expected, 2);

    ASSERT_TRUE(bn_modexp(&r, &base, &exponent, &modulus));
    ASSERT_EQ(bn_cmp(&r, &expected), 0);
}

TEST(bignum, modexp_with_exponent_zero_is_one)
{
    bignum base;
    bignum exponent;
    bignum modulus;
    bignum r;
    bignum one;

    ASSERT_TRUE(fromHex(&base, "0123456789abcdef"));
    bn_zero(&exponent);
    ASSERT_TRUE(fromHex(&modulus, "fedcba9876543211"));
    bn_set_u32(&one, 1);

    ASSERT_TRUE(bn_modexp(&r, &base, &exponent, &modulus));
    ASSERT_EQ(bn_cmp(&r, &one), 0);
}

TEST(bignum, gcd_of_coprime_values_is_one)
{
    bignum a;
    bignum b;
    bignum r;
    bignum one;

    bn_set_u32(&a, 17);
    bn_set_u32(&b, 31);
    bn_set_u32(&one, 1);

    ASSERT_TRUE(bn_gcd(&r, &a, &b));
    ASSERT_EQ(bn_cmp(&r, &one), 0);
}

TEST(bignum, gcd_finds_a_shared_factor)
{
    bignum a;
    bignum b;
    bignum r;
    bignum expected;

    bn_set_u32(&a, 48);
    bn_set_u32(&b, 18);
    bn_set_u32(&expected, 6);

    ASSERT_TRUE(bn_gcd(&r, &a, &b));
    ASSERT_EQ(bn_cmp(&r, &expected), 0);
}

TEST(bignum, modinv_produces_a_true_inverse)
{
    bignum a;
    bignum m;
    bignum inverse;
    bignum product;
    bignum one;

    ASSERT_TRUE(fromHex(&a, "0123456789abcdef"));
    ASSERT_TRUE(fromHex(&m, "fedcba9876543211"));
    bn_set_u32(&one, 1);

    ASSERT_TRUE(bn_modinv(&inverse, &a, &m));
    ASSERT_TRUE(bn_mulmod(&product, &a, &inverse, &m));
    ASSERT_EQ(bn_cmp(&product, &one), 0);
}

TEST(bignum, small_primes_are_recognised)
{
    bignum a;

    bn_set_u32(&a, 65537);
    ASSERT_TRUE(bn_is_probable_prime(&a, 8, testRng));

    bn_set_u32(&a, 65539);
    ASSERT_TRUE(bn_is_probable_prime(&a, 8, testRng));
}

TEST(bignum, composites_are_rejected)
{
    bignum a;

    bn_set_u32(&a, 65536);
    ASSERT_FALSE(bn_is_probable_prime(&a, 8, testRng));

    bn_set_u32(&a, 3233); // 61 * 53
    ASSERT_FALSE(bn_is_probable_prime(&a, 8, testRng));
}

TEST(bignum, from_bytes_rejects_an_oversized_input)
{
    bignum a;
    uint8_t huge[BN_MAX_WORDS * 4 + 8];
    memset(huge, 0xFF, sizeof(huge));

    ASSERT_FALSE(bn_from_bytes(&a, huge, sizeof(huge)));
}
