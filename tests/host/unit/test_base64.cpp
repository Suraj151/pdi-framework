/******************************* Base64 Tests *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <utility/Base64.h>

/**
 * RFC 4648 test vectors, which pin every padding case
 */
TEST(base64, encodes_rfc4648_vectors)
{
    struct
    {
        const char *plain;
        const char *encoded;
    } vectors[] = {
        {"f", "Zg=="},
        {"fo", "Zm8="},
        {"foo", "Zm9v"},
        {"foob", "Zm9vYg=="},
        {"fooba", "Zm9vYmE="},
        {"foobar", "Zm9vYmFy"},
    };

    for (uint8_t i = 0; i < (sizeof(vectors) / sizeof(vectors[0])); i++)
    {
        char input[16];
        char out[32];
        memset(out, 0, sizeof(out));
        strcpy(input, vectors[i].plain);

        ASSERT_TRUE(base64Encode(input, (int)strlen(input), out));
        ASSERT_STREQ(out, vectors[i].encoded);
    }
}

TEST(base64, decodes_rfc4648_vectors)
{
    struct
    {
        const char *encoded;
        const char *plain;
    } vectors[] = {
        {"Zg==", "f"},
        {"Zm8=", "fo"},
        {"Zm9v", "foo"},
        {"Zm9vYg==", "foob"},
        {"Zm9vYmE=", "fooba"},
        {"Zm9vYmFy", "foobar"},
    };

    for (uint8_t i = 0; i < (sizeof(vectors) / sizeof(vectors[0])); i++)
    {
        unsigned char out[32];
        memset(out, 0, sizeof(out));

        int len = base64Decode(vectors[i].encoded, (int)strlen(vectors[i].encoded), out);
        ASSERT_EQ(len, (int)strlen(vectors[i].plain));
        ASSERT_MEMEQ(out, vectors[i].plain, (size_t)len);
    }
}

TEST(base64, round_trips_binary_including_nul)
{
    char input[] = {(char)0x00, (char)0xff, (char)0x10, (char)0x7f, (char)0x80, (char)0x01};
    char encoded[32];
    unsigned char decoded[16];

    memset(encoded, 0, sizeof(encoded));
    memset(decoded, 0, sizeof(decoded));

    ASSERT_TRUE(base64Encode(input, (int)sizeof(input), encoded));

    int len = base64Decode(encoded, (int)strlen(encoded), decoded);
    ASSERT_EQ(len, (int)sizeof(input));
    ASSERT_MEMEQ(decoded, input, sizeof(input));
}

TEST(base64, decode_skips_embedded_whitespace)
{
    unsigned char out[16];
    memset(out, 0, sizeof(out));

    const char *wrapped = "Zm9v\r\nYmFy";
    int len = base64Decode(wrapped, (int)strlen(wrapped), out);
    ASSERT_EQ(len, 6);
    ASSERT_MEMEQ(out, "foobar", 6);
}

TEST(base64, decode_rejects_invalid_alphabet)
{
    unsigned char out[16];
    ASSERT_EQ(base64Decode("Zm9v!!!!", 8, out), -1);
}

TEST(base64, encodes_empty_input_as_empty)
{
    char input[1] = {0};
    char out[8];
    memset(out, 0, sizeof(out));

    base64Encode(input, 0, out);
    ASSERT_STREQ(out, "");
}

TEST(base64, unique_key_fills_requested_length)
{
    char key[17];
    memset(key, 0, sizeof(key));

    genUniqueKey(key, 16);
    ASSERT_EQ(strlen(key), (size_t)16);
}

TEST(base64, unique_key_varies_between_calls)
{
    char first[17];
    char second[17];
    memset(first, 0, sizeof(first));
    memset(second, 0, sizeof(second));

    genUniqueKey(first, 16);
    genUniqueKey(second, 16);
    ASSERT_STRNE(first, second);
}
