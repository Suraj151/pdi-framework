/******************************* UDP Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 25th July 2026
******************************************************************************/

#ifndef _ESP8266_UDP_INTERFACE_H_
#define _ESP8266_UDP_INTERFACE_H_

#include "esp8266.h"
#include <interface/pdi/middlewares/iUdpInterface.h>

extern "C" {
  #include "lwip/udp.h"
  #include "lwip/igmp.h"
  #include "lwip/pbuf.h"
}

#ifndef UDP_RX_BUFFER_SIZE
#define UDP_RX_BUFFER_SIZE 768
#endif

/**
 * UdpInterface class — raw lwIP UDP (esp8266 is single-threaded, no core lock).
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
