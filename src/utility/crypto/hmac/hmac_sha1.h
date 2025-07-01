/***************************** hmac_sha1 ***********************************

This is free software. you can redistribute it and/or modify it but without any
warranty.

referred from   : https://github.com/kokke/tiny-AES-c
added Date      : 1st June 2025
added by        : Suraj I.
******************************************************************************/

#ifndef _HMAC_SHA1_H_
#define _HMAC_SHA1_H_

#include <utility/crypto/hash/sha1.h>
#include <stdint.h>
#include <string.h>

/**
 * Computes HMAC-SHA1
 * @param key Pointer to the HMAC key
 * @param key_len Length of the HMAC key
 * @param data Pointer to the data to be hashed
 * @param data_len Length of the data to be hashed
 * @param output Pointer to the output buffer for the HMAC
 */
void hmac_sha1(const uint8_t *key, unsigned int key_len, const uint8_t *data, unsigned int data_len, uint8_t *output){
    uint8_t k_ipad[64] = {0};
    uint8_t k_opad[64] = {0};
    uint8_t tk[20];
    uint8_t temp[20];
    size_t i;

    // If key is longer than block size, hash it
    if (key_len > 64) {
        pdiutil::SHA1((char*)tk, (const char*)key, key_len);
        key = tk;
        key_len = 20;
    }

    // Copy key into pads
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    // XOR key with ipad and opad values
    for (i = 0; i < 64; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    // Inner SHA1
    // SHA1(k_ipad || data)
    uint8_t inner[64 + data_len];
    memcpy(inner, k_ipad, 64);
    memcpy(inner + 64, data, data_len);
    pdiutil::SHA1((char*)temp, (const char*)inner, 64 + data_len);

    // Outer SHA1
    // SHA1(k_opad || inner_hash)
    uint8_t outer[64 + 20];
    memcpy(outer, k_opad, 64);
    memcpy(outer + 64, temp, 20);
    pdiutil::SHA1((char*)output, (const char*)outer, 64 + 20);
}

#endif // _HMAC_SHA1_H_
