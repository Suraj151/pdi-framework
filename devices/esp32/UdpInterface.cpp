/******************************* UDP Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 25th July 2026
******************************************************************************/

#include "UdpInterface.h"

// Acquire the lwIP core lock for the duration of a scope, but only when the
// current thread isn't already the lock holder (the receive callback runs on
// the TCPIP thread with the lock held, so re-locking would deadlock).
struct CoreLockGuard {
  bool m_locked;
  CoreLockGuard() : m_locked(false) {
#if LWIP_TCPIP_CORE_LOCKING
    if (!sys_thread_tcpip(LWIP_CORE_LOCK_QUERY_HOLDER)) {
      LOCK_TCPIP_CORE();
      m_locked = true;
    }
#endif
  }
  ~CoreLockGuard() {
#if LWIP_TCPIP_CORE_LOCKING
    if (m_locked) UNLOCK_TCPIP_CORE();
#endif
  }
};

UdpInterface::UdpInterface() :
  m_pcb(nullptr),
  m_port(0),
  m_on_packet(nullptr)
{
}

UdpInterface::~UdpInterface() {
  close();
}

bool UdpInterface::begin(uint16_t local_port) {

  close();

  CoreLockGuard guard;

  m_pcb = udp_new();
  if (nullptr == m_pcb) return false;

  m_port = local_port;

  // link-local multicast (mDNS/SSDP) expects TTL 255
  m_pcb->ttl = 255;
#if LWIP_MULTICAST_TX_OPTIONS
  udp_set_multicast_ttl(m_pcb, 255);
#endif

  if (ERR_OK != udp_bind(m_pcb, IP4_ADDR_ANY, local_port)) {
    udp_remove(m_pcb);
    m_pcb = nullptr;
    return false;
  }

  udp_recv(m_pcb, &UdpInterface::lwipRecvCb, this);
  return true;
}

bool UdpInterface::joinMulticastGroup(const ipaddress_t &group) {

  ipaddress_t g = group;
  ip4_addr_t grp;
  IP4_ADDR(&grp, g[0], g[1], g[2], g[3]);

  CoreLockGuard guard;
  return (ERR_OK == igmp_joingroup(IP4_ADDR_ANY4, &grp));
}

int32_t UdpInterface::send(const uint8_t *data, uint16_t len, const ipaddress_t &dst, uint16_t dst_port) {

  if (nullptr == m_pcb || nullptr == data || 0 == len) return PDI_ERR_INVALID_ARG;

  struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
  if (nullptr == p) return PDI_ERR_NO_MEM;

  err_t pe = pbuf_take(p, data, len);
  if (ERR_OK != pe) {
    pbuf_free(p);
    return PDI_ERR_FROM_LWIP(pe);
  }

  ipaddress_t d = dst;
  ip_addr_t dstaddr;
  IP_ADDR4(&dstaddr, d[0], d[1], d[2], d[3]);

  err_t e;
  {
    CoreLockGuard guard;
    e = udp_sendto(m_pcb, p, &dstaddr, dst_port);
  }
  pbuf_free(p);

  return (ERR_OK == e) ? (int32_t)len : PDI_ERR_FROM_LWIP(e);
}

void UdpInterface::setOnPacketCallback(CallBackVoidPointerArgFn callbk) {
  m_on_packet = callbk;
}

void UdpInterface::close() {
  if (nullptr != m_pcb) {
    CoreLockGuard guard;
    udp_recv(m_pcb, nullptr, nullptr);
    udp_remove(m_pcb);
    m_pcb = nullptr;
  }
}

void UdpInterface::lwipRecvCb(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {

  UdpInterface *self = (UdpInterface *)arg;
  if (nullptr == p) return;

  if (nullptr != self && self->m_on_packet && nullptr != addr) {

    uint16_t len = p->tot_len;
    if (len > UDP_RX_BUFFER_SIZE) len = UDP_RX_BUFFER_SIZE;
    pbuf_copy_partial(p, self->m_rxbuf, len, 0);

    const ip4_addr_t *a4 = ip_2_ip4(addr);
    udp_packet_t pkt;
    pkt.m_data = self->m_rxbuf;
    pkt.m_len = len;
    pkt.m_src_ip = ipaddress_t(ip4_addr1(a4), ip4_addr2(a4), ip4_addr3(a4), ip4_addr4(a4));
    pkt.m_src_port = port;

    self->m_on_packet((void *)&pkt);
  }

  pbuf_free(p);
}
