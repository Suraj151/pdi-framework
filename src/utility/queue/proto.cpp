#include "proto.h"

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
 * @return 0 on success.
 */
char PROTO_Init(PROTO_PARSER *parser, PROTO_PARSE_CALLBACK *completeCallback, unsigned char *buf, uint16_t bufSize)
{
    parser->buf = buf;
    parser->bufSize = bufSize;
    parser->dataLen = 0;
    parser->callback = completeCallback;
    parser->isEsc = 0;
    return 0;
}

/**
 * @brief Parses a single byte using the protocol parser.
 *
 * This function processes a single byte of data and updates the parser's state.
 * It handles escape sequences, packet start, and packet end markers.
 *
 * @param parser Pointer to the `PROTO_PARSER` structure.
 * @param value The byte to parse.
 * @return 0 if a complete packet is parsed, -1 otherwise.
 */
char PROTO_ParseByte(PROTO_PARSER *parser, unsigned char value)
{
    switch (value)
    {
    case 0x7D: // Escape character
        parser->isEsc = 1;
        break;

    case 0x7E: // Start of packet
        parser->dataLen = 0;
        parser->isEsc = 0;
        parser->isBegin = 1;
        break;

    case 0x7F: // End of packet
        if (parser->callback != NULL)
            parser->callback();
        parser->isBegin = 0;
        return 0;
        break;

    default:
        if (parser->isBegin == 0)
            break;

        if (parser->isEsc)
        {
            value ^= 0x20; // Unescape the value
            parser->isEsc = 0;
        }

        if (parser->dataLen < parser->bufSize)
            parser->buf[parser->dataLen++] = value;

        break;
    }
    return -1;
}

/**
 * @brief Parses a data buffer using the protocol parser.
 *
 * This function processes a data buffer and extracts protocol packets based on
 * the parser's state.
 *
 * @param parser Pointer to the `PROTO_PARSER` structure.
 * @param buf Pointer to the data buffer to parse.
 * @param len Length of the data buffer.
 * @return 0 on success.
 */
char PROTO_Parse(PROTO_PARSER *parser, unsigned char *buf, uint16_t len)
{
    while (len--)
        PROTO_ParseByte(parser, *buf++);

    return 0;
}

/**
 * @brief Parses data from a ring buffer.
 *
 * This function extracts protocol packets from a ring buffer.
 *
 * @param rb Pointer to the `RINGBUF` structure.
 * @param bufOut Pointer to the buffer to store the parsed data.
 * @param len Pointer to store the length of the parsed data.
 * @param maxBufLen Maximum length of the output buffer.
 * @return 0 if a complete packet is parsed, -1 otherwise.
 */
int PROTO_ParseRb(RINGBUF *rb, unsigned char *bufOut, uint16_t *len, uint16_t maxBufLen)
{
    unsigned char c;

    PROTO_PARSER proto;
    PROTO_Init(&proto, NULL, bufOut, maxBufLen);
    while (RINGBUF_Get(rb, &c) == 0)
    {
        if (PROTO_ParseByte(&proto, c) == 0)
        {
            *len = proto.dataLen;
            return 0;
        }
    }
    return -1;
}

/**
 * @brief Adds a packet to a buffer.
 *
 * This function appends a protocol packet to a buffer, escaping special characters
 * as needed.
 *
 * @param buf Pointer to the buffer to add the packet to.
 * @param packet Pointer to the packet data.
 * @param bufSize Size of the buffer.
 * @return The number of bytes added to the buffer, or -1 on failure.
 */
int PROTO_Add(unsigned char *buf, const unsigned char *packet, int bufSize)
{
    uint16_t i = 2;
    uint16_t len = *(uint16_t *)packet;

    if (bufSize < 1)
        return -1;

    *buf++ = 0x7E; // Start of packet
    bufSize--;

    while (len--)
    {
        switch (*packet)
        {
        case 0x7D:
        case 0x7E:
        case 0x7F:
            if (bufSize < 2)
                return -1;
            *buf++ = 0x7D; // Escape character
            *buf++ = *packet++ ^ 0x20; // Escaped value
            i += 2;
            bufSize -= 2;
            break;
        default:
            if (bufSize < 1)
                return -1;
            *buf++ = *packet++;
            i++;
            bufSize--;
            break;
        }
    }

    if (bufSize < 1)
        return -1;
    *buf++ = 0x7F; // End of packet

    return i;
}

/**
 * @brief Adds a packet to a ring buffer.
 *
 * This function appends a protocol packet to a ring buffer, escaping special
 * characters as needed.
 *
 * @param rb Pointer to the `RINGBUF` structure.
 * @param packet Pointer to the packet data.
 * @param len Length of the packet data.
 * @return The number of bytes added to the ring buffer, or -1 on failure.
 */
int PROTO_AddRb(RINGBUF *rb, const unsigned char *packet, int len)
{
    uint16_t i = 2;
    if (RINGBUF_Put(rb, 0x7E) == -1) // Start of packet
        return -1;
    while (len--)
    {
        switch (*packet)
        {
        case 0x7D:
        case 0x7E:
        case 0x7F:
            if (RINGBUF_Put(rb, 0x7D) == -1) // Escape character
                return -1;
            if (RINGBUF_Put(rb, *packet++ ^ 0x20) == -1) // Escaped value
                return -1;
            i += 2;
            break;
        default:
            if (RINGBUF_Put(rb, *packet++) == -1)
                return -1;
            i++;
            break;
        }
    }
    if (RINGBUF_Put(rb, 0x7F) == -1) // End of packet
        return -1;

    return i;
}
