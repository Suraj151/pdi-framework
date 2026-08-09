/**************************** mDNS service ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 25th July 2026
******************************************************************************/

#ifndef _MDNS_SERVICE_PROVIDER_H_
#define _MDNS_SERVICE_PROVIDER_H_

#include <service_provider/ServiceProvider.h>

#ifdef ENABLE_MDNS_SERVICE

/**
 * MdnsServiceProvider class
 *
 * A minimal multicast-DNS + DNS-SD responder built directly on iUdpInterface
 * (lwIP UDP, no vendor mDNS library). Answers `<hostname>.local` A queries and
 * advertises registered services (e.g. _http._tcp, _ssh._tcp) via PTR/SRV/TXT
 * on 224.0.0.251:5353 so the device is reachable by name and discoverable in
 * service browsers. The responder (re)starts whenever the station gets an IP.
 */
class MdnsServiceProvider : public ServiceProvider
{

public:
  MdnsServiceProvider();
  ~MdnsServiceProvider();

  bool initService(void *arg = nullptr) override;
  bool stopService() override;
  void printStatusToTerminal(iTerminalInterface *terminal) override;

  /* register a DNS-SD service to advertise */
  bool addService(const char *type, const char *proto, uint16_t port);

  /* (re)open the UDP socket and announce; called on station-got-IP */
  bool startResponder(void);

  const char *getHostname(void) { return m_hostname.c_str(); }

protected:
  void onUdpPacket(void *arg);
  void announce(void);
  void sendAResponse(void);
  void sendServiceEnumeration(void);
  void sendServiceBundle(uint8_t svc_idx);
  void buildHostname(void);
  void ensureServerCertificate(void);
  void provisionServerCertificate(void);

  /* wire-format name writers — each returns bytes written */
  uint8_t writeLabels(uint8_t *out, const char *const *labels, uint8_t n);
  uint8_t writeHostName(uint8_t *out);
  uint8_t writeServiceTypeName(uint8_t *out, uint8_t svc_idx);
  uint8_t writeInstanceName(uint8_t *out, uint8_t svc_idx);

  iUdpInterface  *m_udp;
  pdiutil::string m_hostname;
  ipaddress_t     m_ip;
  mdns_service_t  m_services[MDNS_MAX_SERVICES];
  uint8_t         m_service_count;
#ifdef ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME
  bool            m_cert_task_pending;
#endif
};

extern MdnsServiceProvider __mdns_service;

#endif

#endif
