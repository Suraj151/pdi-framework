/*************************** Network Config page *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef _NETWORK_CONFIG_H_
#define _NETWORK_CONFIG_H_

#include "Common.h"

/**
 * network configurations for device communication
 */

#ifndef HOSTS_FILE_PATH
#define HOSTS_FILE_PATH "/etc/hosts"
#endif

#ifndef HOSTS_FILE_SEED
#define HOSTS_FILE_SEED "127.0.0.1 localhost\n"
#endif

#ifndef DNS_RESOLVE_TIMEOUT_MS
#define DNS_RESOLVE_TIMEOUT_MS 5000
#endif

#endif
