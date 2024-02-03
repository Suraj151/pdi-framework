/*
 * File:   proto.h
 * Author: ThuHien
 *
 * Created on November 23, 2012, 8:57 AM
 */

#ifndef _PROTO_H_
#define _PROTO_H_

#include "ringbuf.h"

typedef void(PROTO_PARSE_CALLBACK)();

typedef struct
{
	unsigned char *buf;
	uint16_t bufSize;
	uint16_t dataLen;
	unsigned char isEsc;
	unsigned char isBegin;
	PROTO_PARSE_CALLBACK *callback;
} PROTO_PARSER;

char PROTO_Init(PROTO_PARSER *parser, PROTO_PARSE_CALLBACK *completeCallback, unsigned char *buf, uint16_t bufSize);
char PROTO_Parse(PROTO_PARSER *parser, unsigned char *buf, uint16_t len);
int PROTO_Add(unsigned char *buf, const unsigned char *packet, int bufSize);
int PROTO_AddRb(RINGBUF *rb, const unsigned char *packet, int len);
char PROTO_ParseByte(PROTO_PARSER *parser, unsigned char value);
int PROTO_ParseRb(RINGBUF *rb, unsigned char *bufOut, uint16_t *len, uint16_t maxBufLen);
#endif
