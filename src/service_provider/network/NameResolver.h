/******************************** Name Resolver *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 23rd July 2026
******************************************************************************/

#ifndef _NAME_RESOLVER_H_
#define _NAME_RESOLVER_H_

#include <service_provider/ServiceProvider.h>

#ifdef ENABLE_NETWORK_SERVICE

/**
 * NameResolver
 *
 * Resolves a hostname to an IPv4 address, trying in order:
 *   IP literal -> /etc/hosts -> DNS (iWiFiInterface::hostByName).
 */
class NameResolver {

public:

  static void ensureHostsFile();
  static bool resolve(const char *hostname, ipaddress_t &out, uint32_t timeout_ms = DNS_RESOLVE_TIMEOUT_MS);
  static bool lookupHostsFile(const char *hostname, ipaddress_t &out);
  static bool parseIpLiteral(const char *str, ipaddress_t &out);
};

#endif

#endif
