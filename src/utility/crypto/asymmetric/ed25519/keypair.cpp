#include "ed25519.h"
#include <utility/crypto/hash/sha512.h>
#include "ge.h"


void ed25519_create_keypair(unsigned char *public_key, unsigned char *private_key, const unsigned char *seed) {
    ge_p3 A;

    sha512(seed, 32, private_key);
    private_key[0] &= 248;
    private_key[31] &= 63;
    private_key[31] |= 64;

    ge_scalarmult_base(&A, private_key);
    ge_p3_tobytes(public_key, &A);
}

void ed25519_private_to_curve25519(const unsigned char *ed25519_priv, unsigned char *curve25519_priv) {
    uint8_t h[64];
    // Hash the first 32 bytes of the Ed25519 private key
    sha512(ed25519_priv, 32, h);
    h[0]  &= 248;
    h[31] &= 127;
    h[31] |= 64;
    // Copy the modified hash to the Curve25519 private key
    for (size_t i = 0; i < 32; i++){
        curve25519_priv[i] = h[i];
    }
}