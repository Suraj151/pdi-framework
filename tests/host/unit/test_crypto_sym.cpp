/************************ Symmetric Crypto Tests ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

AES vectors from FIPS 197 and NIST SP 800-38A. HMAC vectors from RFC 4231 and
RFC 2202.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <utility/DataTypeConversions.h>
#include <utility/crypto/hmac/hmac_sha1.h>
#include <utility/crypto/hmac/hmac_sha256.h>
#include <utility/crypto/symmetric/aes/aes.h>

TEST(aes, ecb_encrypts_the_fips197_vector)
{
    const uint8_t key[16] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                             0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
    uint8_t block[16] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
                         0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};
    const uint8_t expected[16] = {0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
                                  0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97};

    struct AES_ctx ctx;
    AES_init_ctx(&ctx, key);
    AES_ECB_encrypt(&ctx, block);

    ASSERT_MEMEQ(block, expected, 16);
}

TEST(aes, ecb_decrypt_reverses_encrypt)
{
    const uint8_t key[16] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                             0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
    const uint8_t plain[16] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
                               0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};
    uint8_t block[16];
    memcpy(block, plain, 16);

    struct AES_ctx ctx;
    AES_init_ctx(&ctx, key);
    AES_ECB_encrypt(&ctx, block);
    ASSERT_FALSE(0 == memcmp(block, plain, 16));

    AES_ECB_decrypt(&ctx, block);
    ASSERT_MEMEQ(block, plain, 16);
}

TEST(aes, cbc_encrypts_the_sp800_38a_vector)
{
    const uint8_t key[16] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                             0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
    const uint8_t iv[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    uint8_t buf[32] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
                       0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
                       0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
                       0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51};
    const uint8_t expected[32] = {0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46,
                                  0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d,
                                  0x50, 0x86, 0xcb, 0x9b, 0x50, 0x72, 0x19, 0xee,
                                  0x95, 0xdb, 0x11, 0x3a, 0x91, 0x76, 0x78, 0xb2};

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_encrypt_buffer(&ctx, buf, sizeof(buf));

    ASSERT_MEMEQ(buf, expected, 32);
}

TEST(aes, cbc_round_trips)
{
    const uint8_t key[16] = {0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
                             0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81};
    const uint8_t iv[16] = {0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
                            0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff};
    uint8_t original[32];
    uint8_t buf[32];

    for (uint8_t i = 0; i < 32; i++)
    {
        original[i] = (uint8_t)(i * 7);
    }
    memcpy(buf, original, sizeof(buf));

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_encrypt_buffer(&ctx, buf, sizeof(buf));

    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_decrypt_buffer(&ctx, buf, sizeof(buf));

    ASSERT_MEMEQ(buf, original, sizeof(original));
}

TEST(aes, ctr_is_its_own_inverse)
{
    const uint8_t key[16] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                             0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
    const uint8_t iv[16] = {0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
                            0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff};
    const char *message = "pdi framework counter mode payload";
    uint8_t buf[40];
    uint8_t original[40];

    memset(buf, 0, sizeof(buf));
    memcpy(buf, message, strlen(message));
    memcpy(original, buf, sizeof(buf));

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CTR_xcrypt_buffer(&ctx, buf, sizeof(buf));
    ASSERT_FALSE(0 == memcmp(buf, original, sizeof(buf)));

    AES_init_ctx_iv(&ctx, key, iv);
    AES_CTR_xcrypt_buffer(&ctx, buf, sizeof(buf));
    ASSERT_MEMEQ(buf, original, sizeof(original));
}

TEST(hmac, sha256_matches_rfc4231_case1)
{
    uint8_t key[20];
    uint8_t mac[32];
    char hex[80];

    memset(key, 0x0b, sizeof(key));
    hmac_sha256(key, sizeof(key), (const uint8_t *)"Hi There", 8, mac);

    BytesToHexString(mac, 32, hex);
    ASSERT_STREQ(hex, "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
}

TEST(hmac, sha256_matches_rfc4231_case2)
{
    const char *key = "Jefe";
    const char *data = "what do ya want for nothing?";
    uint8_t mac[32];
    char hex[80];

    hmac_sha256((const uint8_t *)key, (unsigned int)strlen(key),
                (const uint8_t *)data, (unsigned int)strlen(data), mac);

    BytesToHexString(mac, 32, hex);
    ASSERT_STREQ(hex, "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
}

TEST(hmac, sha256_handles_a_key_longer_than_the_block)
{
    uint8_t key[131];
    uint8_t mac[32];
    char hex[80];

    memset(key, 0xaa, sizeof(key));
    hmac_sha256(key, sizeof(key), (const uint8_t *)"Test Using Larger Than Block-Size Key - Hash Key First", 54, mac);

    BytesToHexString(mac, 32, hex);
    ASSERT_STREQ(hex, "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");
}

TEST(hmac, sha1_matches_rfc2202_case2)
{
    const char *key = "Jefe";
    const char *data = "what do ya want for nothing?";
    uint8_t mac[20];
    char hex[48];

    hmac_sha1((const uint8_t *)key, (unsigned int)strlen(key),
              (const uint8_t *)data, (unsigned int)strlen(data), mac);

    BytesToHexString(mac, 20, hex);
    ASSERT_STREQ(hex, "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79");
}

TEST(hmac, differs_when_the_key_differs)
{
    uint8_t keya[16];
    uint8_t keyb[16];
    uint8_t maca[32];
    uint8_t macb[32];

    memset(keya, 0x01, sizeof(keya));
    memset(keyb, 0x02, sizeof(keyb));

    hmac_sha256(keya, sizeof(keya), (const uint8_t *)"same data", 9, maca);
    hmac_sha256(keyb, sizeof(keyb), (const uint8_t *)"same data", 9, macb);

    ASSERT_FALSE(0 == memcmp(maca, macb, 32));
}
