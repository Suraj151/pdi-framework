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
#ifndef SSH_MAX_SESSIONS
#define SSH_MAX_SESSIONS 2
#endif

#define SSH_DEFAULT_DIR ".ssh"
#define SSH_KEY_ALGO_ED25519_STR "ed25519"
#define SSH_KEY_ALGO_RSA_STR "rsa"

#define SSH_AUTHORIZED_KEYS_FILE CHARPTR_WRAP("authorized_keys")
#define SSH_ED25519_KEY_TYPE_STR CHARPTR_WRAP("ssh-ed25519")
#define SSH_ED25519_SIG_SIZE 64

/* SSH server configuration file and its option keys */
#define SSH_CONFIG_DIR CHARPTR_WRAP("/etc/ssh")
#define SSH_CONFIG_FILE CHARPTR_WRAP("/etc/ssh/sshconfig")
#define SSH_CONFIG_KEY_PASSWORD_AUTH CHARPTR_WRAP("PasswordAuthentication")
#define SSH_CONFIG_KEY_PUBKEY_AUTH CHARPTR_WRAP("PubkeyAuthentication")

/* SSH Key algorithm options */
enum SSHKeyAlgorithm{
    SSH_KEY_ALGO_MIN = 0,
    SSH_KEY_ALGO_ED25519,
    // SSH_KEY_ALGO_RSA,
    SSH_KEY_ALGO_MAX
};

/* SSH server auth policy, populated from SSH_CONFIG_FILE.
   Both methods default enabled when the file is absent. */
typedef struct ssh_config {
    bool m_password_auth;
    bool m_pubkey_auth;
    ssh_config() : m_password_auth(true), m_pubkey_auth(true) {}
} ssh_config_t;

#endif
