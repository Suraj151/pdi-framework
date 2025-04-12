/*
 * File:   proto.h
 * Author: ThuHien
 *
 * Created on November 23, 2012, 8:57 AM
 *
 * This file provides utilities for handling protocol parsing and packet
 * management. It includes functions for initializing parsers, parsing data,
 * and managing packets using ring buffers.
 */

#ifndef _PROTO_H_
#define _PROTO_H_

#include "ringbuf.h"

/**
 * @typedef PROTO_PARSE_CALLBACK
 * @brief Defines a callback function type for protocol parsing completion.
 */
typedef void(PROTO_PARSE_CALLBACK)();

/**
 * @struct PROTO_PARSER
 * @brief Represents a protocol parser structure.
 *
 * This structure is used for parsing incoming data streams and managing
 * protocol-specific parsing states.
 */
typedef struct
{
    unsigned char *buf;                ///< Pointer to the buffer for storing parsed data.
    uint16_t bufSize;                  ///< Size of the buffer.
    uint16_t dataLen;                  ///< Length of the data currently in the buffer.
    unsigned char isEsc;               ///< Flag indicating if the parser is in an escape state.
    unsigned char isBegin;             ///< Flag indicating if the parser has started parsing a packet.
    PROTO_PARSE_CALLBACK *callback;    ///< Callback function to invoke when parsing is complete.
} PROTO_PARSER;

/**
 * @brief Initializes a protocol parser.
 *
 * This function sets up a protocol parser with a buffer and a callback function
 * to handle completed parsing events.
 *
 * @param parser Pointer to the `PROTO_PARSER` structure to initialize.
 * @param completeCallback Callback function to invoke when parsing is complete.
 * @param buf Pointer to the buffer for storing parsed data.
 * @param bufSize Size of the buffer.
 * @return 0 on success, -1 on failure.
 */
char PROTO_Init(PROTO_PARSER *parser, PROTO_PARSE_CALLBACK *completeCallback, unsigned char *buf, uint16_t bufSize);

/**
 * @brief Parses a data buffer using the protocol parser.
 *
 * This function processes a data buffer and extracts protocol packets based on
 * the parser's state.
 *
 * @param parser Pointer to the `PROTO_PARSER` structure.
 * @param buf Pointer to the data buffer to parse.
 * @param len Length of the data buffer.
 * @return 0 on success, -1 on failure.
 */
char PROTO_Parse(PROTO_PARSER *parser, unsigned char *buf, uint16_t len);

/**
 * @brief Adds a packet to a buffer.
 *
 * This function appends a protocol packet to a buffer.
 *
 * @param buf Pointer to the buffer to add the packet to.
 * @param packet Pointer to the packet data.
 * @param bufSize Size of the buffer.
 * @return The number of bytes added to the buffer, or -1 on failure.
 */
int PROTO_Add(unsigned char *buf, const unsigned char *packet, int bufSize);

/**
 * @brief Adds a packet to a ring buffer.
 *
 * This function appends a protocol packet to a ring buffer.
 *
 * @param rb Pointer to the `RINGBUF` structure.
 * @param packet Pointer to the packet data.
 * @param len Length of the packet data.
 * @return The number of bytes added to the ring buffer, or -1 on failure.
 */
int PROTO_AddRb(RINGBUF *rb, const unsigned char *packet, int len);

/**
 * @brief Parses a single byte using the protocol parser.
 *
 * This function processes a single byte of data and updates the parser's state.
 *
 * @param parser Pointer to the `PROTO_PARSER` structure.
 * @param value The byte to parse.
 * @return 0 on success, -1 on failure.
 */
char PROTO_ParseByte(PROTO_PARSER *parser, unsigned char value);

/**
 * @brief Parses data from a ring buffer.
 *
 * This function extracts protocol packets from a ring buffer.
 *
 * @param rb Pointer to the `RINGBUF` structure.
 * @param bufOut Pointer to the buffer to store the parsed data.
 * @param len Pointer to store the length of the parsed data.
 * @param maxBufLen Maximum length of the output buffer.
 * @return The number of bytes parsed, or -1 on failure.
 */
int PROTO_ParseRb(RINGBUF *rb, unsigned char *bufOut, uint16_t *len, uint16_t maxBufLen);

#endif
