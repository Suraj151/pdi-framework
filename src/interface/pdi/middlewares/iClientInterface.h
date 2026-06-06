/***************************** Client Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_CLIENT_INTERFACE_H_
#define _I_CLIENT_INTERFACE_H_

#include <utility/iIOInterface.h>

/**
 * iClientInterface class
 */
class iClientInterface : public iTerminalInterface
{

public:
  /**
   * iClientInterface constructor.
   */
  iClientInterface() {}
  /**
   * iClientInterface destructor.
   */
  virtual ~iClientInterface() {}

  // useful api
  virtual void setTimeout(uint32_t timeout) = 0;
};

/**
 * iTcpClientInterface class
 */
class iTcpClientInterface : public iClientInterface
{

public:
  /**
   * iTcpClientInterface constructor.
   */
  iTcpClientInterface() {}

  /**
   * iTcpClientInterface destructor.
   */
  virtual ~iTcpClientInterface() {}

  /**
   * @brief Get the local IP address.
   * @return The local IP address.
   */
  virtual ipaddress_t getLocalIp() const = 0;

  /**
   * @brief Get the local port.
   * @return The local port number.
   */
  virtual uint16_t getLocalPort() const = 0;

  /**
   * @brief Get the remote IP address.
   * @return The remote IP address.
   */
  virtual ipaddress_t getRemoteIp() const = 0;

  /**
   * @brief Get the remote port.
   * @return The remote port number.
   */
  virtual uint16_t getRemotePort() const = 0;

  /**
   * @brief Enable TCP keep-alive and configure its parameters.
   * @param idleTime The idle time (in seconds) before sending the first keep-alive packet.
   * @param interval The interval (in seconds) between keep-alive packets.
   * @param count The number of keep-alive packets to send before considering the connection dead.
   * @return True if keep-alive was successfully enabled, false otherwise.
   */
  virtual bool setKeepAlive(uint16_t idleTime, uint16_t interval, uint16_t count) = 0;

  /**
   * @brief Set the NoDelay option for TCP.
   * @param noDelay If true, disables Nagle's algorithm (reducing latency).
   */
  virtual void setNoDelay(bool noDelay) = 0;
};


#ifdef ENABLE_TLS_SERVICE

/**
 * iTlsClientInterface class
 *
 * A TLS-secured TCP client. Inherits the full iTcpClientInterface contract so
 * that any consumer written against iTcpClientInterface (HTTP, MQTT, SMTP, ...)
 * works transparently over TLS once the cert material is configured.
 */
class iTlsClientInterface : public iTcpClientInterface
{

public:
  /**
   * iTlsClientInterface constructor.
   */
  iTlsClientInterface() {}

  /**
   * iTlsClientInterface destructor.
   */
  virtual ~iTlsClientInterface() {}

  /**
   * @brief Set the path to the trusted Certificate Authority used to verify the server.
   * @param path Filesystem path to a PEM/DER-encoded CA certificate.
   * @return True on success, false otherwise.
   */
  virtual bool setCertificateAuthorityPath(const char* path) = 0;

  /**
   * @brief Set the path to the client certificate for mutual TLS (optional).
   * @param path Filesystem path to a PEM/DER-encoded client certificate.
   * @return True on success, false otherwise.
   */
  virtual bool setClientCertificatePath(const char* path) { return false; }

  /**
   * @brief Set the path to the client private key for mutual TLS (optional).
   * @param path Filesystem path to a PEM/DER-encoded private key.
   * @return True on success, false otherwise.
   */
  virtual bool setClientPrivateKeyPath(const char* path) { return false; }

  /**
   * @brief Is secure flasg to identify client is secure.
   */
  virtual bool isSecure() { return true; }

  /**
   * @brief Set the SNI hostname presented during the TLS handshake.
   * @param hostname Null-terminated server name.
   */
  virtual void setSNIHostname(const char* hostname) {}

  /**
   * @brief Toggle peer certificate verification at runtime.
   *
   * Note: this does not change whether the transport itself is TLS -
   * isSecure() still reports the transport capability. This flag controls
   * only whether the peer's certificate chain is validated against the
   * configured trust anchors. The handshake always runs the full key
   * exchange and the bulk cipher is always negotiated, so traffic stays
   * encrypted in either mode.
   *
   * @param verify true  - verify the peer cert chain (default; recommended
   *                       for production and any path that crosses an
   *                       untrusted network).
   *               false - accept any cert without validation. Suitable
   *                       only for development / bench testing - leaves
   *                       the session vulnerable to active MITM attacks.
   */
  virtual void setVerifyPeer(bool verify) {}
};

#endif


// Using typedefs for easier reference
using iUdpClientInterface = iClientInterface;

#endif
