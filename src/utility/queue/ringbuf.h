#ifndef _RING_BUF_H_
#define _RING_BUF_H_

#include <utility/DataTypeDef.h>

typedef struct
{
	unsigned char *p_o;				/**< Original pointer */
	unsigned char *volatile p_r;	/**< Read pointer */
	unsigned char *volatile p_w;	/**< Write pointer */
	volatile unsigned int fill_cnt; /**< Number of filled slots */
	unsigned int size;				/**< Buffer size */
} RINGBUF;

int RINGBUF_Init(RINGBUF *r, unsigned char *buf, unsigned int size);
int RINGBUF_Put(RINGBUF *r, unsigned char c);
int RINGBUF_Get(RINGBUF *r, unsigned char *c);
#endif
