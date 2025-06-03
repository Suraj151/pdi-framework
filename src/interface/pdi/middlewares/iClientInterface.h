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


// Using typedefs for easier reference
using iUdpClientInterface = iClientInterface;

#endif
