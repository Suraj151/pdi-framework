/*********************** Asymmetric Crypto Tests ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Ed25519 vectors from RFC 8032 section 7.1, X25519 vectors from RFC 7748
section 6.1, cross checked against an independent implementation.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <utility/DataTypeConversions.h>
#include <utility/crypto/asymmetric/curve25519/curve25519.h>
#include <utility/crypto/asymmetric/ed25519/ed25519.h>

static void fromHex(const char *hex, uint8_t *out, uint8_t bytelen)
{
    HexStringToBytes(hex, bytelen, out);
}

TEST(ed25519, derives_the_rfc8032_test1_public_key)
{
    uint8_t seed[32];
    uint8_t pub[ED25519_PUBKEY_SIZE];
    uint8_t priv[ED25519_PRIVKEY_SIZE];
    char hex[80];

    fromHex("9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60", seed, 32);
    ed25519_create_keypair(pub, priv, seed);

    BytesToHexString(pub, 32, hex);
    ASSERT_STREQ(hex, "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a");
}

TEST(ed25519, signs_the_rfc8032_test1_empty_message)
{
    uint8_t seed[32];
    uint8_t pub[ED25519_PUBKEY_SIZE];
    uint8_t priv[ED25519_PRIVKEY_SIZE];
    uint8_t sig[64];
    char hex[144];

    fromHex("9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60", seed, 32);
    ed25519_create_keypair(pub, priv, seed);
    ed25519_sign(sig, nullptr, 0, pub, priv);

    BytesToHexString(sig, 64, hex);
    ASSERT_STREQ(hex,
                 "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e06522490155"
                 "5fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b");
}

TEST(ed25519, derives_the_rfc8032_test2_public_key)
{
    uint8_t seed[32];
    uint8_t pub[ED25519_PUBKEY_SIZE];
    uint8_t priv[ED25519_PRIVKEY_SIZE];
    char hex[80];

    fromHex("4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb", seed, 32);
    ed25519_create_keypair(pub, priv, seed);

    BytesToHexString(pub, 32, hex);
    ASSERT_STREQ(hex, "3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c");
}

TEST(ed25519, signs_the_rfc8032_test2_single_byte_message)
{
    uint8_t seed[32];
    uint8_t pub[ED25519_PUBKEY_SIZE];
    uint8_t priv[ED25519_PRIVKEY_SIZE];
    uint8_t sig[64];
    const uint8_t message[1] = {0x72};
    char hex[144];

    fromHex("4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb", seed, 32);
    ed25519_create_keypair(pub, priv, seed);
    ed25519_sign(sig, message, sizeof(message), pub, priv);

    BytesToHexString(sig, 64, hex);
    ASSERT_STREQ(hex,
                 "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da"
                 "085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00");
}

TEST(ed25519, verifies_its_own_signature)
{
    uint8_t seed[32];
    uint8_t pub[ED25519_PUBKEY_SIZE];
    uint8_t priv[ED25519_PRIVKEY_SIZE];
    uint8_t sig[64];
    const char *message = "pdi framework host key material";

    memset(seed, 0x11, sizeof(seed));
    ed25519_create_keypair(pub, priv, seed);
    ed25519_sign(sig, (const uint8_t *)message, strlen(message), pub, priv);

    ASSERT_EQ(ed25519_verify(sig, (const uint8_t *)message, strlen(message), pub), 1);
}

TEST(ed25519, rejects_a_tampered_signature)
{
    uint8_t seed[32];
    uint8_t pub[ED25519_PUBKEY_SIZE];
    uint8_t priv[ED25519_PRIVKEY_SIZE];
    uint8_t sig[64];
    const char *message = "authentic message";

    memset(seed, 0x22, sizeof(seed));
    ed25519_create_keypair(pub, priv, seed);
    ed25519_sign(sig, (const uint8_t *)message, strlen(message), pub, priv);

    sig[0] ^= 0x01;
    ASSERT_EQ(ed25519_verify(sig, (const uint8_t *)message, strlen(message), pub), 0);
}

TEST(ed25519, rejects_a_tampered_message)
{
    uint8_t seed[32];
    uint8_t pub[ED25519_PUBKEY_SIZE];
    uint8_t priv[ED25519_PRIVKEY_SIZE];
    uint8_t sig[64];
    char message[32];

    strcpy(message, "authentic message");
    memset(seed, 0x33, sizeof(seed));
    ed25519_create_keypair(pub, priv, seed);
    ed25519_sign(sig, (const uint8_t *)message, strlen(message), pub, priv);

    message[0] = 'A';
    ASSERT_EQ(ed25519_verify(sig, (const uint8_t *)message, strlen(message), pub), 0);
}

TEST(ed25519, rejects_a_signature_from_a_different_key)
{
    uint8_t seeda[32];
    uint8_t seedb[32];
    uint8_t puba[ED25519_PUBKEY_SIZE];
    uint8_t priva[ED25519_PRIVKEY_SIZE];
    uint8_t pubb[ED25519_PUBKEY_SIZE];
    uint8_t privb[ED25519_PRIVKEY_SIZE];
    uint8_t sig[64];
    const char *message = "signed by a";

    memset(seeda, 0x44, sizeof(seeda));
    memset(seedb, 0x55, sizeof(seedb));
    ed25519_create_keypair(puba, priva, seeda);
    ed25519_create_keypair(pubb, privb, seedb);

    ed25519_sign(sig, (const uint8_t *)message, strlen(message), puba, priva);
    ASSERT_EQ(ed25519_verify(sig, (const uint8_t *)message, strlen(message), pubb), 0);
}

TEST(ed25519, signing_is_deterministic)
{
    uint8_t seed[32];
    uint8_t pub[ED25519_PUBKEY_SIZE];
    uint8_t priv[ED25519_PRIVKEY_SIZE];
    uint8_t first[64];
    uint8_t second[64];
    const char *message = "same message every time";

    memset(seed, 0x66, sizeof(seed));
    ed25519_create_keypair(pub, priv, seed);
    ed25519_sign(first, (const uint8_t *)message, strlen(message), pub, priv);
    ed25519_sign(second, (const uint8_t *)message, strlen(message), pub, priv);

    ASSERT_MEMEQ(first, second, 64);
}

TEST(ed25519, distinct_seeds_give_distinct_keys)
{
    uint8_t seeda[32];
    uint8_t seedb[32];
    uint8_t puba[ED25519_PUBKEY_SIZE];
    uint8_t priva[ED25519_PRIVKEY_SIZE];
    uint8_t pubb[ED25519_PUBKEY_SIZE];
    uint8_t privb[ED25519_PRIVKEY_SIZE];

    memset(seeda, 0x01, sizeof(seeda));
    memset(seedb, 0x02, sizeof(seedb));
    ed25519_create_keypair(puba, priva, seeda);
    ed25519_create_keypair(pubb, privb, seedb);

    ASSERT_FALSE(0 == memcmp(puba, pubb, ED25519_PUBKEY_SIZE));
}

TEST(ed25519, key_exchange_agrees_between_both_sides)
{
    uint8_t seeda[32];
    uint8_t seedb[32];
    uint8_t puba[ED25519_PUBKEY_SIZE];
    uint8_t priva[ED25519_PRIVKEY_SIZE];
    uint8_t pubb[ED25519_PUBKEY_SIZE];
    uint8_t privb[ED25519_PRIVKEY_SIZE];
    uint8_t shareda[32];
    uint8_t sharedb[32];

    memset(seeda, 0x77, sizeof(seeda));
    memset(seedb, 0x88, sizeof(seedb));
    ed25519_create_keypair(puba, priva, seeda);
    ed25519_create_keypair(pubb, privb, seedb);

    ed25519_key_exchange(shareda, pubb, priva);
    ed25519_key_exchange(sharedb, puba, privb);

    ASSERT_MEMEQ(shareda, sharedb, 32);
}

TEST(curve25519, derives_the_rfc7748_alice_public_key)
{
    uint8_t priv[CURVE25519_PRIVKEY_SIZE];
    uint8_t pub[CURVE25519_PUBKEY_SIZE];
    char hex[80];

    fromHex("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a", priv, 32);
    crypto_scalarmult_base(pub, priv);

    BytesToHexString(pub, 32, hex);
    ASSERT_STREQ(hex, "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a");
}

TEST(curve25519, derives_the_rfc7748_bob_public_key)
{
    uint8_t priv[CURVE25519_PRIVKEY_SIZE];
    uint8_t pub[CURVE25519_PUBKEY_SIZE];
    char hex[80];

    fromHex("5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb", priv, 32);
    crypto_scalarmult_base(pub, priv);

    BytesToHexString(pub, 32, hex);
    ASSERT_STREQ(hex, "de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f");
}

TEST(curve25519, reaches_the_rfc7748_shared_secret)
{
    uint8_t privalice[32];
    uint8_t pubbob[32];
    uint8_t shared[32];
    char hex[80];

    fromHex("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a", privalice, 32);
    fromHex("de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f", pubbob, 32);
    crypto_scalarmult(shared, privalice, pubbob);

    BytesToHexString(shared, 32, hex);
    ASSERT_STREQ(hex, "4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742");
}

TEST(curve25519, both_sides_agree_on_the_shared_secret)
{
    uint8_t privalice[32];
    uint8_t privbob[32];
    uint8_t pubalice[32];
    uint8_t pubbob[32];
    uint8_t shareda[32];
    uint8_t sharedb[32];

    fromHex("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a", privalice, 32);
    fromHex("5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb", privbob, 32);

    crypto_scalarmult_base(pubalice, privalice);
    crypto_scalarmult_base(pubbob, privbob);

    crypto_scalarmult(shareda, privalice, pubbob);
    crypto_scalarmult(sharedb, privbob, pubalice);

    ASSERT_MEMEQ(shareda, sharedb, 32);
}

TEST(curve25519, generated_keypairs_agree)
{
    uint8_t privalice[CURVE25519_PRIVKEY_SIZE];
    uint8_t pubalice[CURVE25519_PUBKEY_SIZE];
    uint8_t privbob[CURVE25519_PRIVKEY_SIZE];
    uint8_t pubbob[CURVE25519_PUBKEY_SIZE];
    uint8_t shareda[32];
    uint8_t sharedb[32];

    curve25519_create_keypair(pubalice, privalice);
    curve25519_create_keypair(pubbob, privbob);

    crypto_scalarmult(shareda, privalice, pubbob);
    crypto_scalarmult(sharedb, privbob, pubalice);

    ASSERT_MEMEQ(shareda, sharedb, 32);
}
