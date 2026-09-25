/****************************** Instance Interface *****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 31st Aug 2026
******************************************************************************/

#include "iInstanceInterface.h"
#include <interface/pdi/middlewares/iClientInterface.h>

#ifdef ENABLE_NETWORK_SERVICE

#include "SafeAlloc.h"

/**
 * The one outbound client the services share, built on first call and kept, so
 * a service can source it instead of being handed it.
 */
iTcpClientInterface* iInstanceInterface::getSharedTcpClientInstance()
{
  if (nullptr == m_shared_tcp_client) {
    m_shared_tcp_client = getNewTcpClientInstance();
  }

  return m_shared_tcp_client;
}

/**
 * Releases the shared client, for the stack shutting down; a service that stops
 * leaves it for the services still running.
 */
void iInstanceInterface::releaseSharedTcpClientInstance()
{
  pdiutil::safe_delete(m_shared_tcp_client);
}

#ifdef ENABLE_TLS_SERVICE

/**
 * The one outbound TLS client the services share, built on first call and kept,
 * so a service can source it instead of being handed it.
 */
iTlsClientInterface* iInstanceInterface::getSharedTlsClientInstance()
{
  if (nullptr == m_shared_tls_client) {
    m_shared_tls_client = getNewTlsClientInstance();

    // To verify the servers this client connects to, point the line below at a
    // CA bundle on the device filesystem and drop the setVerifyPeer(false).
    // Default here is encrypted-but-unverified so no cert needs provisioning.
    // m_shared_tls_client->setCertificateAuthorityPath(TLS_DEFAULT_OUTBOUND_CA_BUNDLE_PATH);
    if (nullptr != m_shared_tls_client && m_shared_tls_client->isSecure()) {
      m_shared_tls_client->setVerifyPeer(false);
    }
  }

  return m_shared_tls_client;
}

/**
 * Releases the shared TLS client, for the stack shutting down; a service that
 * stops leaves it for the services still running.
 */
void iInstanceInterface::releaseSharedTlsClientInstance()
{
  pdiutil::safe_delete(m_shared_tls_client);
}

#endif

#endif

/**
 * The shared client that carries the named protocol, null where this build
 * has none of that kind.
 */
iClientInterface* iInstanceInterface::getSharedClientForScheme(protocol_scheme_t scheme)
{
  switch (scheme) {

#ifdef ENABLE_NETWORK_SERVICE
    case PROTO_SCHEME_HTTP:
    case PROTO_SCHEME_MQTT:
    case PROTO_SCHEME_SMTP:
      return getSharedTcpClientInstance();
#endif

#ifdef ENABLE_TLS_SERVICE
    case PROTO_SCHEME_HTTPS:
    case PROTO_SCHEME_MQTTS:
    case PROTO_SCHEME_SMTPS:
      return getSharedTlsClientInstance();
#endif

    default:
      break;
  }

  return nullptr;
}
