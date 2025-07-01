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
#define SSH_DEFAULT_DIR ".ssh"
#define SSH_KEY_ALGO_ED25519_STR "ed25519"
#define SSH_KEY_ALGO_RSA_STR "rsa"

/* SSH Key algorithm options */
enum SSHKeyAlgorithm{
    SSH_KEY_ALGO_MIN = 0,
    SSH_KEY_ALGO_ED25519,
    // SSH_KEY_ALGO_RSA,
    SSH_KEY_ALGO_MAX
};

#endif
