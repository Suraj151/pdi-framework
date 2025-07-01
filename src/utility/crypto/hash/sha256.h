#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>

#define SHA256_BLOCK_SIZE 32            // SHA256 outputs a 32 byte digest

/**************************** DATA TYPES ****************************/
// typedef unsigned char BYTE;             // 8-bit byte
// typedef unsigned int  WORD;             // 32-bit word, change to "long" for 16-bit machines

typedef struct {
	unsigned char data[64];
	unsigned int datalen;
	unsigned long long bitlen;
	unsigned int state[8];
} sha256_context;

/*********************** FUNCTION DECLARATIONS **********************/
void sha256_init(sha256_context *ctx);
void sha256_update(sha256_context *ctx, const unsigned char data[], unsigned int len);
void sha256_final(sha256_context *ctx, unsigned char hash[]);
void sha256(const unsigned char *message, unsigned int message_len, unsigned char *out);

#endif   // SHA256_H