/******************************** mDNS Config ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 25th July 2026
******************************************************************************/
#ifndef _MDNS_CONFIG_H_
#define _MDNS_CONFIG_H_

#include "Common.h"

/**
 * mDNS responder configuration
 */

#ifndef MDNS_PORT
#define MDNS_PORT 5353
#endif

#ifndef MDNS_RECORD_TTL
#define MDNS_RECORD_TTL 120
#endif

#ifndef MDNS_HOSTNAME_MAXLEN
#define MDNS_HOSTNAME_MAXLEN 32
#endif

#ifndef MDNS_HOSTNAME_PREFIX
#define MDNS_HOSTNAME_PREFIX "pdi-"
#endif

// link-local mDNS multicast group (224.0.0.251)
#ifndef MDNS_MULTICAST_ADDR_0
#define MDNS_MULTICAST_ADDR_0 224
#endif
#ifndef MDNS_MULTICAST_ADDR_1
#define MDNS_MULTICAST_ADDR_1 0
#endif
#ifndef MDNS_MULTICAST_ADDR_2
#define MDNS_MULTICAST_ADDR_2 0
#endif
#ifndef MDNS_MULTICAST_ADDR_3
#define MDNS_MULTICAST_ADDR_3 251
#endif

// DNS wire-format record types / classes
#ifndef MDNS_TYPE_A
#define MDNS_TYPE_A 1
#endif
#ifndef MDNS_TYPE_PTR
#define MDNS_TYPE_PTR 12
#endif
#ifndef MDNS_TYPE_TXT
#define MDNS_TYPE_TXT 16
#endif
#ifndef MDNS_TYPE_SRV
#define MDNS_TYPE_SRV 33
#endif
#ifndef MDNS_TYPE_ANY
#define MDNS_TYPE_ANY 255
#endif
#ifndef MDNS_CLASS_IN
#define MDNS_CLASS_IN 1
#endif
#ifndef MDNS_CACHE_FLUSH
#define MDNS_CACHE_FLUSH 0x8000
#endif
#ifndef MDNS_LOCAL_LABEL
#define MDNS_LOCAL_LABEL "local"
#endif
#ifndef MDNS_LOCAL_LABEL_LEN
#define MDNS_LOCAL_LABEL_LEN 5
#endif

// DNS-SD service advertisement
#ifndef MDNS_MAX_SERVICES
#define MDNS_MAX_SERVICES 5
#endif
#ifndef MDNS_PTR_TTL
#define MDNS_PTR_TTL 4500
#endif
#ifndef MDNS_TX_BUFFER_SIZE
#define MDNS_TX_BUFFER_SIZE 320
#endif

#endif
