/******************************* UDP Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 25th July 2026
******************************************************************************/

#ifndef _ESP32_UDP_INTERFACE_H_
#define _ESP32_UDP_INTERFACE_H_

#include "esp32.h"
#include <interface/pdi/middlewares/iUdpInterface.h>

extern "C" {
  #include "lwip/udp.h"
  #include "lwip/igmp.h"
  #include "lwip/pbuf.h"
  #include "lwip/tcpip.h"
}

#ifndef UDP_RX_BUFFER_SIZE
#define UDP_RX_BUFFER_SIZE 768
#endif

/**
 * UdpInterface class — raw lwIP UDP.
 *
 * esp32 lwIP runs multi-threaded with TCPIP core locking, so raw udp_* calls
 * must hold the core lock. The receive callback already runs on the TCPIP
 * thread (lock held), so replies sent from it must NOT re-lock — the internal
 * guard locks only when the current thread isn't already the lock holder.
 */
class UdpInterface : public iUdpInterface {

  public:

    UdpInterface();
    ~UdpInterface();

    bool begin(uint16_t local_port) override;
    bool joinMulticastGroup(const ipaddress_t &group) override;
    int32_t send(const uint8_t *data, uint16_t len, const ipaddress_t &dst, uint16_t dst_port) override;
    void setOnPacketCallback(CallBackVoidPointerArgFn callbk) override;
    void close() override;

  protected:

    static void lwipRecvCb(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

    struct udp_pcb          *m_pcb;
    uint16_t                 m_port;
    CallBackVoidPointerArgFn m_on_packet;
    uint8_t                  m_rxbuf[UDP_RX_BUFFER_SIZE];
};

#endif
