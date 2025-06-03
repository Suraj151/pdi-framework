/* queue.cpp --
 *
 * Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * * Neither the name of Redis nor the names of its contributors may be used
 * to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "queue.h"

/**
 * @brief Initializes a queue.
 *
 * This function allocates memory for the queue's buffer and initializes the
 * associated ring buffer.
 *
 * @param queue Pointer to the `QUEUE` structure to initialize.
 * @param bufferSize The size of the memory buffer to allocate for the queue.
 */
void QUEUE_Init(QUEUE *queue, int bufferSize)
{
    queue->buf = new uint8_t[bufferSize];
    RINGBUF_Init(&queue->rb, queue->buf, bufferSize);
}

/**
 * @brief Adds data to the queue.
 *
 * This function writes data to the queue using the ring buffer. If the queue
 * is full, the function will return an error.
 *
 * @param queue Pointer to the `QUEUE` structure.
 * @param buffer Pointer to the data to add to the queue.
 * @param len The length of the data to add.
 * @return The number of bytes written to the queue, or -1 if the queue is full.
 */
int32_t QUEUE_Puts(QUEUE *queue, uint8_t *buffer, uint16_t len)
{
    return PROTO_AddRb(&queue->rb, buffer, len);
}

/**
 * @brief Retrieves data from the queue.
 *
 * This function reads data from the queue using the ring buffer. If the queue
 * is empty, the function will return an error.
 *
 * @param queue Pointer to the `QUEUE` structure.
 * @param buffer Pointer to the buffer to store the retrieved data.
 * @param len Pointer to store the length of the retrieved data.
 * @param maxLen The maximum length of data to retrieve.
 * @return The number of bytes read from the queue, or -1 if the queue is empty.
 */
int32_t QUEUE_Gets(QUEUE *queue, uint8_t *buffer, uint16_t *len, uint16_t maxLen)
{
    return PROTO_ParseRb(&queue->rb, buffer, len, maxLen);
}

/**
 * @brief Checks if the queue is empty.
 *
 * This function checks whether the queue is empty by examining the fill count
 * of the ring buffer.
 *
 * @param queue Pointer to the `QUEUE` structure.
 * @return True if the queue is empty, false otherwise.
 */
bool QUEUE_IsEmpty(QUEUE *queue)
{
    if (queue->rb.fill_cnt <= 0)
        return true;
    return false;
}
