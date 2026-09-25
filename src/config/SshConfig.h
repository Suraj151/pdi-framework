/***************************** SSH Config page *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2025
******************************************************************************/
#ifndef _SSH_CONFIG_H_
#define _SSH_CONFIG_H_

#include "Common.h"

/**
 * SSH configurations for secure communication
 */
#ifndef SSH_DEFAULT_PORT
#define SSH_DEFAULT_PORT 22
#endif

#ifndef SSH_CLIENT_VERSION_MAX
#define SSH_CLIENT_VERSION_MAX 255   /* longest identification string the protocol allows */
#endif

#ifndef SSH_MAX_SESSIONS
#define SSH_MAX_SESSIONS 2
#endif

/**
 * How long a client may wait for a pool slot before it is refused. A slot
 * frees in well under a second, so a short wait serves a client that would
 * otherwise be turned away while one is being reclaimed.
 */
#ifndef SSH_POOL_FULL_GRACE_MS
#define SSH_POOL_FULL_GRACE_MS 5000
#endif

#ifndef SSH_HANDSHAKE_IDLE_MS
#define SSH_HANDSHAKE_IDLE_MS 10000
#endif

#ifndef SSH_SFTP_IDLE_MS
#define SSH_SFTP_IDLE_MS 120000
#endif

#ifndef SSH_SHELL_IDLE_MS
#define SSH_SHELL_IDLE_MS 500000
#endif

#ifndef SSH_CHANNEL_CLOSE_GRACE_MS
#define SSH_CHANNEL_CLOSE_GRACE_MS 3000
#endif

#define SSH_DEFAULT_DIR ".ssh"
#define SSH_KEY_ALGO_ED25519_STR "ed25519"
#define SSH_KEY_ALGO_RSA_STR "rsa"

#define SSH_AUTHORIZED_KEYS_FILE "authorized_keys"
#define SSH_ED25519_KEY_TYPE_STR "ssh-ed25519"
#define SSH_ED25519_SIG_SIZE 64

#define SSH_RSA_KEY_TYPE_STR "ssh-rsa"
#define SSH_RSA_SIG_ALGO_SHA256_STR "rsa-sha2-256"
#define SSH_RSA_SIG_ALGO_SHA512_STR "rsa-sha2-512"

#define SSH_EXT_INFO_C_STR "ext-info-c"
#define SSH_EXT_INFO_S_STR "ext-info-s"
#define SSH_EXT_SERVER_SIG_ALGS_STR "server-sig-algs"
#define SSH_SERVER_SIG_ALGS_VALUE_STR "rsa-sha2-512,rsa-sha2-256,ssh-ed25519"

#ifndef SSH_RSA_KEY_BITS
#define SSH_RSA_KEY_BITS 2048
#endif

/* SSH server configuration file and its option keys */
#define SSH_CONFIG_DIR "/etc/ssh"
/* Server host keys live with the server config, ~/.ssh holds the user's client keys */
#define SSH_HOST_KEY_DIR SSH_CONFIG_DIR
#define SSH_CONFIG_KEY_PASSWORD_AUTH "PasswordAuthentication"
#define SSH_CONFIG_KEY_PUBKEY_AUTH "PubkeyAuthentication"
#define SSH_CONFIG_HEADER \
    "# PDI SSH server configuration" TERMINAL_NEW_LINE \
    "# PasswordAuthentication yes|no" TERMINAL_NEW_LINE \
    "# PubkeyAuthentication yes|no" TERMINAL_NEW_LINE

/* Private host key material, readable only by its owner like the user store */
#ifndef SSH_PRIVATE_KEY_PERMS
#define SSH_PRIVATE_KEY_PERMS 0600
#endif

/* SSH Key algorithm options */
enum SSHKeyAlgorithm{
    SSH_KEY_ALGO_MIN = 0,
    SSH_KEY_ALGO_ED25519,
    SSH_KEY_ALGO_RSA_SHA256,
    SSH_KEY_ALGO_RSA_SHA512,
    SSH_KEY_ALGO_MAX
};

/* SSH server auth policy, populated from the service config file.
   Both methods default enabled when the file is absent. */
typedef struct ssh_config {
    bool m_password_auth;
    bool m_pubkey_auth;
    ssh_config() : m_password_auth(true), m_pubkey_auth(true) {}
} ssh_config_t;

#endif
