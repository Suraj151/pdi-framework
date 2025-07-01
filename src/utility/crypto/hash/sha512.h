#ifndef SHA512_H
#define SHA512_H

#include <stddef.h>
#include <utility/crypto/fixedint.h>

/* state */
typedef struct sha512_context_ {
    uint64_t  length, state[8];
    unsigned int curlen;
    unsigned char buf[128];
} sha512_context;


int sha512_init(sha512_context * md);
int sha512_final(sha512_context * md, unsigned char *out);
int sha512_update(sha512_context * md, const unsigned char *in, unsigned int inlen);
int sha512(const unsigned char *message, unsigned int message_len, unsigned char *out);

#endif
