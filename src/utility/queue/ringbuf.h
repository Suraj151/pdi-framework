#ifndef _RING_BUF_H_
#define _RING_BUF_H_

#include <utility/DataTypeDef.h>

/**
 * @struct RINGBUF
 * @brief Represents a ring buffer structure.
 *
 * A ring buffer (circular buffer) is a fixed-size buffer that operates in a FIFO
 * (First In, First Out) manner. It is commonly used for data streaming or buffering
 * where data is continuously written and read.
 */
typedef struct
{
    unsigned char *p_o;				/**< Original pointer to the buffer memory */
    unsigned char *volatile p_r;	/**< Read pointer indicating the next byte to read */
    unsigned char *volatile p_w;	/**< Write pointer indicating the next byte to write */
    volatile unsigned int fill_cnt; /**< Number of filled slots in the buffer */
    unsigned int size;				/**< Total size of the buffer */
} RINGBUF;

/**
 * @brief Initializes a ring buffer.
 *
 * This function sets up the ring buffer by associating it with a memory buffer
 * and defining its size.
 *
 * @param r Pointer to the RINGBUF structure to initialize.
 * @param buf Pointer to the memory buffer to use for the ring buffer.
 * @param size Size of the memory buffer.
 * @return 0 on success, -1 on failure.
 */
int RINGBUF_Init(RINGBUF *r, unsigned char *buf, unsigned int size);

/**
 * @brief Adds a byte to the ring buffer.
 *
 * This function writes a byte to the ring buffer. If the buffer is full, the
 * function will return an error.
 *
 * @param r Pointer to the RINGBUF structure.
 * @param c The byte to add to the buffer.
 * @return 0 on success, -1 if the buffer is full.
 */
int RINGBUF_Put(RINGBUF *r, unsigned char c);

/**
 * @brief Retrieves a byte from the ring buffer.
 *
 * This function reads a byte from the ring buffer. If the buffer is empty, the
 * function will return an error.
 *
 * @param r Pointer to the RINGBUF structure.
 * @param c Pointer to store the retrieved byte.
 * @return 0 on success, -1 if the buffer is empty.
 */
int RINGBUF_Get(RINGBUF *r, unsigned char *c);

#endif
