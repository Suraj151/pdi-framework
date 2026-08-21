/******************************** Hash Tests **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Known answer vectors from FIPS 180-4 and RFC 6234.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <utility/DataTypeConversions.h>
#include <utility/crypto/hash/sha1.h>
#include <utility/crypto/hash/sha256.h>
#include <utility/crypto/hash/sha512.h>

static void digestToHex(const uint8_t *digest, uint8_t len, char *out)
{
    BytesToHexString(digest, len, out);
}

TEST(sha256, hashes_empty_input)
{
    uint8_t digest[32];
    char hex[80];

    sha256((const unsigned char *)"", 0, digest);
    digestToHex(digest, 32, hex);
    ASSERT_STREQ(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(sha256, hashes_abc)
{
    uint8_t digest[32];
    char hex[80];

    sha256((const unsigned char *)"abc", 3, digest);
    digestToHex(digest, 32, hex);
    ASSERT_STREQ(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(sha256, hashes_two_block_message)
{
    const char *message = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    uint8_t digest[32];
    char hex[80];

    sha256((const unsigned char *)message, (unsigned int)strlen(message), digest);
    digestToHex(digest, 32, hex);
    ASSERT_STREQ(hex, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

TEST(sha256, hashes_million_a_characters)
{
    sha256_context ctx;
    uint8_t block[1000];
    uint8_t digest[32];
    char hex[80];

    memset(block, 'a', sizeof(block));
    sha256_init(&ctx);
    for (int i = 0; i < 1000; i++)
    {
        sha256_update(&ctx, block, sizeof(block));
    }
    sha256_final(&ctx, digest);

    digestToHex(digest, 32, hex);
    ASSERT_STREQ(hex, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

TEST(sha256, incremental_matches_one_shot)
{
    const char *message = "the quick brown fox jumps over the lazy dog";
    uint8_t oneshot[32];
    uint8_t chunked[32];
    sha256_context ctx;

    sha256((const unsigned char *)message, (unsigned int)strlen(message), oneshot);

    sha256_init(&ctx);
    sha256_update(&ctx, (const unsigned char *)message, 10);
    sha256_update(&ctx, (const unsigned char *)message + 10, 20);
    sha256_update(&ctx, (const unsigned char *)message + 30,
                  (unsigned int)strlen(message) - 30);
    sha256_final(&ctx, chunked);

    ASSERT_MEMEQ(chunked, oneshot, 32);
}

TEST(sha256, handles_bytes_above_ascii)
{
    uint8_t input[256];
    uint8_t digest[32];

    for (int i = 0; i < 256; i++)
    {
        input[i] = (uint8_t)i;
    }

    sha256(input, sizeof(input), digest);

    char hex[80];
    digestToHex(digest, 32, hex);
    ASSERT_STREQ(hex, "40aff2e9d2d8922e47afd4648e6967497158785fbd1da870e7110266bf944880");
}

TEST(sha1, hashes_abc)
{
    char digest[20];
    char hex[48];

    pdiutil::SHA1(digest, "abc", 3);
    digestToHex((const uint8_t *)digest, 20, hex);
    ASSERT_STREQ(hex, "a9993e364706816aba3e25717850c26c9cd0d89d");
}

TEST(sha1, hashes_empty_input)
{
    char digest[20];
    char hex[48];

    pdiutil::SHA1(digest, "", 0);
    digestToHex((const uint8_t *)digest, 20, hex);
    ASSERT_STREQ(hex, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
}

TEST(sha1, hashes_two_block_message)
{
    const char *message = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    char digest[20];
    char hex[48];

    pdiutil::SHA1(digest, message, (uint32_t)strlen(message));
    digestToHex((const uint8_t *)digest, 20, hex);
    ASSERT_STREQ(hex, "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
}

TEST(sha1, incremental_matches_one_shot)
{
    const char *message = "pdi framework portable device interface";
    char oneshot[20];
    unsigned char chunked[20];
    pdiutil::SHA1_CTX ctx;

    pdiutil::SHA1(oneshot, message, (uint32_t)strlen(message));

    pdiutil::SHA1Init(&ctx);
    pdiutil::SHA1Update(&ctx, (const unsigned char *)message, 7);
    pdiutil::SHA1Update(&ctx, (const unsigned char *)message + 7, (uint32_t)strlen(message) - 7);
    pdiutil::SHA1Final(chunked, &ctx);

    ASSERT_MEMEQ(chunked, oneshot, 20);
}

TEST(sha512, hashes_empty_input)
{
    uint8_t digest[64];
    char hex[144];

    sha512((const unsigned char *)"", 0, digest);
    digestToHex(digest, 64, hex);
    ASSERT_STREQ(hex,
                 "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce"
                 "47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");
}

TEST(sha512, hashes_abc)
{
    uint8_t digest[64];
    char hex[144];

    sha512((const unsigned char *)"abc", 3, digest);
    digestToHex(digest, 64, hex);
    ASSERT_STREQ(hex,
                 "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
                 "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
}

TEST(sha512, hashes_message_spanning_two_blocks)
{
    const char *message = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
                          "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
    uint8_t digest[64];
    char hex[144];

    sha512((const unsigned char *)message, (unsigned int)strlen(message), digest);
    digestToHex(digest, 64, hex);
    ASSERT_STREQ(hex,
                 "8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb6889018"
                 "501d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909");
}

TEST(sha512, incremental_matches_one_shot)
{
    const char *message = "portable device interface stack";
    uint8_t oneshot[64];
    uint8_t chunked[64];
    sha512_context ctx;

    sha512((const unsigned char *)message, (unsigned int)strlen(message), oneshot);

    sha512_init(&ctx);
    sha512_update(&ctx, (const unsigned char *)message, 8);
    sha512_update(&ctx, (const unsigned char *)message + 8, (unsigned int)strlen(message) - 8);
    sha512_final(&ctx, chunked);

    ASSERT_MEMEQ(chunked, oneshot, 64);
}
