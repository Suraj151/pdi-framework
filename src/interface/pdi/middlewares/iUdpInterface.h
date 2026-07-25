/******************************* UDP Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 25th July 2026
******************************************************************************/

#ifndef _I_UDP_INTERFACE_H_
#define _I_UDP_INTERFACE_H_

#include <interface/interface_includes.h>

// forward declaration of derived class for this interface
class UdpInterface;

/**
 * iUdpInterface class
 *
 * A bound UDP socket abstraction over the platform's lwIP UDP layer. Datagrams
 * arrive via the on-packet callback as a udp_packet_t. Supports IPv4 multicast
 * membership so responders like mDNS can listen on a group address.
 */
class iUdpInterface
{

public:
  /**
   * iUdpInterface constructor.
   */
  iUdpInterface() {}
  /**
   * iUdpInterface destructor.
   */
  virtual ~iUdpInterface() {}

  /**
   * @brief Bind the socket to a local UDP port (0 = ephemeral).
   * @return true on success.
   */
  virtual bool begin(uint16_t local_port) = 0;

  /**
   * @brief Join an IPv4 multicast group so datagrams sent to it are received.
   * @return true on success.
   */
  virtual bool joinMulticastGroup(const ipaddress_t &group) = 0;

  /**
   * @brief Send a datagram to a destination address/port.
   * @return bytes queued, or a negative error code.
   */
  virtual int32_t send(const uint8_t *data, uint16_t len, const ipaddress_t &dst, uint16_t dst_port) = 0;

  /**
   * @brief Register the callback invoked per received datagram (udp_packet_t*).
   */
  virtual void setOnPacketCallback(CallBackVoidPointerArgFn callbk) = 0;

  /**
   * @brief Close the socket and release resources.
   */
  virtual void close() = 0;
};

#endif
