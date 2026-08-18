/***************************** hmac_sha256 *********************************

This is free software. you can redistribute it and/or modify it but without any
warranty.

referred from   : https://github.com/kokke/tiny-AES-c
added Date      : 29th Jul 2025
added by        : Suraj I.
******************************************************************************/

#ifndef _HMAC_SHA256_H_
#define _HMAC_SHA256_H_

#include <utility/crypto/hash/sha256.h>
#include <stdint.h>
#include <string.h>

/**
 * Computes HMAC-SHA256
 * @param key Pointer to the HMAC key
 * @param key_len Length of the HMAC key
 * @param data Pointer to the data to be hashed
 * @param data_len Length of the data to be hashed
 * @param output Pointer to the output buffer for the HMAC (32 bytes)
 */
inline void hmac_sha256(const uint8_t *key, unsigned int key_len, const uint8_t *data, unsigned int data_len, uint8_t *output){
    uint8_t k_ipad[64] = {0};
    uint8_t k_opad[64] = {0};
    uint8_t tk[32];
    uint8_t temp[32];
    size_t i;

    // If key is longer than block size, hash it
    if (key_len > 64) {
        sha256(key, key_len, tk);
        key = tk;
        key_len = 32;
    }

    // Copy key into pads
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    // XOR key with ipad and opad values
    for (i = 0; i < 64; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    // Inner SHA256
    // SHA256(k_ipad || data)
    uint8_t inner[64 + data_len];
    memcpy(inner, k_ipad, 64);
    memcpy(inner + 64, data, data_len);
    sha256(inner, 64 + data_len, temp);

    // Outer SHA256
    // SHA256(k_opad || inner_hash)
    uint8_t outer[64 + 32];
    memcpy(outer, k_opad, 64);
    memcpy(outer + 64, temp, 32);
    sha256(outer, 64 + 32, output);
}

#endif // _HMAC_SHA256_H_
