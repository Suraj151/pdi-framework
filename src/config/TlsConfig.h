/****************************** TLS Config page *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 3rd June 2026
******************************************************************************/
#ifndef _TLS_CONFIG_H_
#define _TLS_CONFIG_H_

#include "Common.h"

/**
 * BearSSL bidirectional IO buffer sizes used per active TLS session.
 *
 * The receive (input) buffer must be large enough to hold a single TLS
 * record from the peer. The default of 6 KB accepts most real-world
 * server-to-browser and browser-to-server traffic where records are
 * sized to TCP MSS (~1460 B) but is *smaller* than BearSSL's
 * BR_SSL_BUFSIZE_INPUT (~17 KB) used by spec-maximum 16 KB records.
 * A peer that emits a 16 KB record will fail the handshake or session.
 *
 * The send (output) buffer caps a single outgoing record. Smaller =>
 * more records on large responses, slightly more CPU, but much less
 * RAM per session. Tradeoff favours RAM on ESP8266 (80 KB total heap).
 *
 * Override per project in your sketch or platform build flags before
 * including pdi-framework headers.
 */
#ifndef TLS_IBUF_SIZE
#define TLS_IBUF_SIZE 2048
#endif

#ifndef TLS_OBUF_SIZE
#define TLS_OBUF_SIZE 1024
#endif

/**
 * Heap-backed cooperative-task stack used by the ESP8266 TLS implementation
 * to run BearSSL operations (ECDSA sign, ECDHE keygen, etc.) outside the
 * user cont_t stack. The user cont_t default (4864 B) is too small to fit
 * an ECDSA P-256 sign call frame; running BearSSL on a dedicated task with
 * an 8 KB stack avoids the stack overflow.
 *
 * Requires ENABLE_CONTEXTUAL_EXECUTION to be defined.
 */
#ifndef TLS_TASK_STACK_SIZE
#define TLS_TASK_STACK_SIZE 6500
#endif

#ifndef TLS_TASK_POLL_MS
#define TLS_TASK_POLL_MS 5
#endif


/**
 * Default filesystem paths for TLS material (Linux FHS-inspired layout).
 *
 * Callers are expected to drop PEM files at these paths on the device's
 * filesystem (LittleFS on ESP8266). The HTTP server / TLS client
 * read from whatever path is passed to set*Path() at runtime; the
 * macros below are just well-known defaults so applications can use
 * a single constant rather than hard-coded literals.
 *
 *   /etc/http/server.crt     HTTPS server certificate (PEM, may be a chain)
 *   /etc/http/server.key     HTTPS server private key (PEM, EC or RSA)
 *   /etc/http/client-ca.crt  CA bundle for mTLS client cert verification
 *   /etc/ssl/ca-bundle.crt   Trust anchors for outbound HTTPS clients
 *
 * These directories must exist on the filesystem before the certs are
 * read; the storage service creates them lazily when needed.
 */
#ifndef TLS_DEFAULT_HTTP_DIR
#define TLS_DEFAULT_HTTP_DIR              "/etc/http"
#endif

#ifndef TLS_DEFAULT_SSL_DIR
#define TLS_DEFAULT_SSL_DIR               "/etc/ssl"
#endif

#ifndef TLS_DEFAULT_SERVER_CERT_PATH
#define TLS_DEFAULT_SERVER_CERT_PATH      TLS_DEFAULT_HTTP_DIR "/server.crt"
#endif

#ifndef TLS_DEFAULT_SERVER_KEY_PATH
#define TLS_DEFAULT_SERVER_KEY_PATH       TLS_DEFAULT_HTTP_DIR "/server.key"
#endif

#ifndef TLS_DEFAULT_CLIENT_CA_PATH
#define TLS_DEFAULT_CLIENT_CA_PATH        TLS_DEFAULT_HTTP_DIR "/client-ca.crt"
#endif

#ifndef TLS_DEFAULT_OUTBOUND_CA_BUNDLE_PATH
#define TLS_DEFAULT_OUTBOUND_CA_BUNDLE_PATH  TLS_DEFAULT_SSL_DIR "/ca-bundle.crt"
#endif

#endif
