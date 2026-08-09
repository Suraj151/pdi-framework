/**************************** mDNS service ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 25th July 2026
******************************************************************************/

#include "MdnsServiceProvider.h"

#ifdef ENABLE_MDNS_SERVICE

#include <interface/pdi/middlewares/iUdpInterface.h>
#include <utility/EventUtil.h>

// ---- small wire-format helpers -------------------------------------------

static void putU16(uint8_t *b, uint16_t &p, uint16_t v) {
  b[p++] = (uint8_t)(v >> 8);
  b[p++] = (uint8_t)(v & 0xFF);
}

static void putU32(uint8_t *b, uint16_t &p, uint32_t v) {
  b[p++] = (uint8_t)(v >> 24);
  b[p++] = (uint8_t)(v >> 16);
  b[p++] = (uint8_t)(v >> 8);
  b[p++] = (uint8_t)(v & 0xFF);
}

// DNS response header: response + authoritative, given answer/additional counts
static void putHeader(uint8_t *b, uint16_t &p, uint16_t ancount, uint16_t arcount) {
  b[p++] = 0;    b[p++] = 0;      // id
  b[p++] = 0x84; b[p++] = 0x00;   // flags: QR + AA
  b[p++] = 0;    b[p++] = 0;      // qdcount
  putU16(b, p, ancount);          // ancount
  b[p++] = 0;    b[p++] = 0;      // nscount
  putU16(b, p, arcount);          // arcount
}

// backfill the 2-byte rdlength placeholder at rdlenPos with (p - rdStart)
static void backfillRdlen(uint8_t *b, uint16_t rdlenPos, uint16_t p) {
  uint16_t rdlen = p - (rdlenPos + 2);
  b[rdlenPos]     = (uint8_t)(rdlen >> 8);
  b[rdlenPos + 1] = (uint8_t)(rdlen & 0xFF);
}

static uint8_t cstrlen(const char *s) {
  uint8_t n = 0;
  while (s[n] != '\0') n++;
  return n;
}

// case-insensitive compare of a wire label (length n) against a C string that
// must be exactly n characters long.
static bool labelEqualsCI(const uint8_t *label, uint8_t n, const char *str) {
  for (uint8_t i = 0; i < n; i++) {
    if (str[i] == '\0') return false;
    uint8_t a = label[i];
    uint8_t c = (uint8_t)str[i];
    if (a >= 'A' && a <= 'Z') a += 32;
    if (c >= 'A' && c <= 'Z') c += 32;
    if (a != c) return false;
  }
  return str[n] == '\0';
}

// --------------------------------------------------------------------------

MdnsServiceProvider::MdnsServiceProvider() : ServiceProvider(SERVICE_MDNS, "MDNS"),
  m_udp(nullptr),
  m_service_count(0)
#ifdef ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME
  , m_cert_task_pending(false)
#endif
{
}

MdnsServiceProvider::~MdnsServiceProvider() {
  stopService();
}

bool MdnsServiceProvider::initService(void *arg) {

  buildHostname();

  // advertise the listening servers this build exposes
  m_service_count = 0;
#ifdef ENABLE_HTTP_SERVER
  #if defined(ENABLE_HTTPS_SERVER) && defined(ENABLE_TLS_SERVICE)
  addService("_https", "_tcp", HTTPS_DEFAULT_PORT);
  #else
  addService("_http", "_tcp", HTTP_DEFAULT_PORT);
  #endif
#endif
#ifdef ENABLE_SSH_SERVICE
  addService("_ssh", "_tcp", 22);
  addService("_sftp-ssh", "_tcp", 22);
#endif
#ifdef ENABLE_TELNET_SERVICE
  addService("_telnet", "_tcp", 23);
#endif

  // (re)start the responder whenever the station acquires an IP
  __utl_event.add_event_listener(EVENT_WIFI_STA_GOT_IP, [](void *a) {
    __mdns_service.m_ip = __i_wifi.localIP();
    __mdns_service.startResponder();
    __mdns_service.ensureServerCertificate();
  });

  // already connected at boot — start now
  if (__i_wifi.localIP().isSet()) {
    m_ip = __i_wifi.localIP();
    startResponder();
    ensureServerCertificate();
  }

  return ServiceProvider::initService(arg);
}

bool MdnsServiceProvider::stopService() {

  if (nullptr != m_udp) {
    m_udp->close();
    pdiutil::safe_delete(m_udp);
    m_udp = nullptr;
  }
#ifdef ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME
  // the base impl drops every tracked task, including a queued cert generation
  m_cert_task_pending = false;
#endif
  return ServiceProvider::stopService();
}

bool MdnsServiceProvider::addService(const char *type, const char *proto, uint16_t port) {

  if (nullptr == type || nullptr == proto) return false;
  if (m_service_count >= MDNS_MAX_SERVICES) return false;

  m_services[m_service_count].m_type = type;
  m_services[m_service_count].m_proto = proto;
  m_services[m_service_count].m_port = port;
  m_service_count++;
  return true;
}

bool MdnsServiceProvider::startResponder() {

  if (nullptr != m_udp) {
    m_udp->close();
    pdiutil::safe_delete(m_udp);
    m_udp = nullptr;
  }

  m_udp = __i_instance.getNewUdpInstance();
  if (nullptr == m_udp) return false; // port without a UDP impl yet

  if (!m_udp->begin(MDNS_PORT)) {
    pdiutil::safe_delete(m_udp);
    m_udp = nullptr;
    return false;
  }

  ipaddress_t mcast(MDNS_MULTICAST_ADDR_0, MDNS_MULTICAST_ADDR_1, MDNS_MULTICAST_ADDR_2, MDNS_MULTICAST_ADDR_3);
  m_udp->joinMulticastGroup(mcast);
  m_udp->setOnPacketCallback([](void *a) { __mdns_service.onUdpPacket(a); });

  announce();
  return true;
}

void MdnsServiceProvider::buildHostname() {

  uint8_t mac[6] = {0};
  __i_wifi.macAddress(mac);
  char buf[MDNS_HOSTNAME_MAXLEN];
  __snprintf(buf, sizeof(buf), "%s%02x%02x%02x", MDNS_HOSTNAME_PREFIX, mac[3], mac[4], mac[5]);
  m_hostname = buf;

#ifdef ENABLE_STORAGE_SERVICE
  // persist to /etc/hostname (overwrite), Linux-style, without the .local suffix
  pdiutil::string content = m_hostname + "\n";
  pdiutil::string hostname_path = CHARPTR_WRAP(HOSTNAME_FILE_PATH);
  __i_fs.writeFile(hostname_path.c_str(), content.c_str(), content.length(), false);
#endif
}

void MdnsServiceProvider::ensureServerCertificate() {

#ifdef ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME
  if (!m_ip.isSet() || m_hostname.empty() || m_cert_task_pending) {
    return;
  }

  // key generation needs several kB of stack and runs for seconds, so it is
  // handed to the scheduler instead of running in the caller context
  m_cert_task_pending = true;
  serviceSetTimeout([]() {
    __mdns_service.m_cert_task_pending = false;
    __mdns_service.provisionServerCertificate();
  }, 1, __i_dvc_ctrl.millis_now());
#endif
}

void MdnsServiceProvider::provisionServerCertificate() {

#ifdef ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME
  if (!m_ip.isSet() || m_hostname.empty()) {
    return;
  }

  // the responder answers on <hostname>.local, so the cert carries it as a
  // subject alt name alongside the address
  pdiutil::string dnsname = m_hostname + "." + MDNS_LOCAL_LABEL;

  if (!__i_fs.isDirExist(TLS_DEFAULT_HTTP_DIR)) {
      __i_fs.createDirectory(TLS_DEFAULT_HTTP_DIR);
  }
  if (!__i_fs.isDirExist(TLS_DEFAULT_SSL_DIR)) {
      __i_fs.createDirectory(TLS_DEFAULT_SSL_DIR);
  }

  TlsCertProvisioner::ensureServerCert(
      TLS_DEFAULT_SERVER_CERT_PATH,
      TLS_DEFAULT_SERVER_KEY_PATH,
      (uint32_t)m_ip,
      dnsname.c_str());
#endif
}

// ---- name writers --------------------------------------------------------

uint8_t MdnsServiceProvider::writeLabels(uint8_t *out, const char *const *labels, uint8_t n) {
  uint8_t o = 0;
  for (uint8_t i = 0; i < n; i++) {
    uint8_t l = cstrlen(labels[i]);
    out[o++] = l;
    for (uint8_t j = 0; j < l; j++) out[o++] = (uint8_t)labels[i][j];
  }
  out[o++] = 0;
  return o;
}

uint8_t MdnsServiceProvider::writeHostName(uint8_t *out) {
  const char *labels[2] = { m_hostname.c_str(), MDNS_LOCAL_LABEL };
  return writeLabels(out, labels, 2);
}

uint8_t MdnsServiceProvider::writeServiceTypeName(uint8_t *out, uint8_t idx) {
  const char *labels[3] = { m_services[idx].m_type, m_services[idx].m_proto, MDNS_LOCAL_LABEL };
  return writeLabels(out, labels, 3);
}

uint8_t MdnsServiceProvider::writeInstanceName(uint8_t *out, uint8_t idx) {
  const char *labels[4] = { m_hostname.c_str(), m_services[idx].m_type, m_services[idx].m_proto, MDNS_LOCAL_LABEL };
  return writeLabels(out, labels, 4);
}

// ---- responders ----------------------------------------------------------

void MdnsServiceProvider::sendAResponse() {

  if (nullptr == m_udp) return;
  uint8_t *resp = pdiutil::safe_new_array<uint8_t>(MDNS_TX_BUFFER_SIZE);
  if (nullptr == resp) return;

  uint16_t p = 0;
  putHeader(resp, p, 1, 0);

  p += writeHostName(resp + p);
  putU16(resp, p, MDNS_TYPE_A);
  putU16(resp, p, MDNS_CLASS_IN | MDNS_CACHE_FLUSH);
  putU32(resp, p, MDNS_RECORD_TTL);
  uint16_t rdlenPos = p; p += 2;
  ipaddress_t ip = m_ip;
  resp[p++] = ip[0]; resp[p++] = ip[1]; resp[p++] = ip[2]; resp[p++] = ip[3];
  backfillRdlen(resp, rdlenPos, p);

  ipaddress_t mcast(MDNS_MULTICAST_ADDR_0, MDNS_MULTICAST_ADDR_1, MDNS_MULTICAST_ADDR_2, MDNS_MULTICAST_ADDR_3);
  m_udp->send(resp, p, mcast, MDNS_PORT);
  pdiutil::safe_delete_array(resp);
}

void MdnsServiceProvider::sendServiceEnumeration() {

  if (nullptr == m_udp || 0 == m_service_count) return;
  uint8_t *resp = pdiutil::safe_new_array<uint8_t>(MDNS_TX_BUFFER_SIZE);
  if (nullptr == resp) return;

  uint16_t p = 0;
  putHeader(resp, p, m_service_count, 0);

  const char *enumLabels[4] = { "_services", "_dns-sd", "_udp", MDNS_LOCAL_LABEL };
  for (uint8_t i = 0; i < m_service_count; i++) {
    p += writeLabels(resp + p, enumLabels, 4);
    putU16(resp, p, MDNS_TYPE_PTR);
    putU16(resp, p, MDNS_CLASS_IN);
    putU32(resp, p, MDNS_PTR_TTL);
    uint16_t rdlenPos = p; p += 2;
    p += writeServiceTypeName(resp + p, i);
    backfillRdlen(resp, rdlenPos, p);
  }

  ipaddress_t mcast(MDNS_MULTICAST_ADDR_0, MDNS_MULTICAST_ADDR_1, MDNS_MULTICAST_ADDR_2, MDNS_MULTICAST_ADDR_3);
  m_udp->send(resp, p, mcast, MDNS_PORT);
  pdiutil::safe_delete_array(resp);
}

void MdnsServiceProvider::sendServiceBundle(uint8_t idx) {

  if (nullptr == m_udp || idx >= m_service_count) return;
  uint8_t *resp = pdiutil::safe_new_array<uint8_t>(MDNS_TX_BUFFER_SIZE);
  if (nullptr == resp) return;

  uint16_t p = 0;
  // PTR in the answer section; SRV + TXT + A as additional records
  putHeader(resp, p, 1, 3);

  // PTR : <type>._tcp.local -> <instance>
  p += writeServiceTypeName(resp + p, idx);
  putU16(resp, p, MDNS_TYPE_PTR);
  putU16(resp, p, MDNS_CLASS_IN);
  putU32(resp, p, MDNS_PTR_TTL);
  {
    uint16_t rdlenPos = p; p += 2;
    p += writeInstanceName(resp + p, idx);
    backfillRdlen(resp, rdlenPos, p);
  }

  // SRV : <instance> -> priority/weight/port/target(<host>.local)
  p += writeInstanceName(resp + p, idx);
  putU16(resp, p, MDNS_TYPE_SRV);
  putU16(resp, p, MDNS_CLASS_IN | MDNS_CACHE_FLUSH);
  putU32(resp, p, MDNS_RECORD_TTL);
  {
    uint16_t rdlenPos = p; p += 2;
    putU16(resp, p, 0);                        // priority
    putU16(resp, p, 0);                        // weight
    putU16(resp, p, m_services[idx].m_port);   // port
    p += writeHostName(resp + p);              // target
    backfillRdlen(resp, rdlenPos, p);
  }

  // TXT : <instance> -> one empty (zero-length) string
  p += writeInstanceName(resp + p, idx);
  putU16(resp, p, MDNS_TYPE_TXT);
  putU16(resp, p, MDNS_CLASS_IN | MDNS_CACHE_FLUSH);
  putU32(resp, p, MDNS_RECORD_TTL);
  putU16(resp, p, 1);
  resp[p++] = 0;

  // A : <host>.local -> ip
  p += writeHostName(resp + p);
  putU16(resp, p, MDNS_TYPE_A);
  putU16(resp, p, MDNS_CLASS_IN | MDNS_CACHE_FLUSH);
  putU32(resp, p, MDNS_RECORD_TTL);
  {
    uint16_t rdlenPos = p; p += 2;
    ipaddress_t ip = m_ip;
    resp[p++] = ip[0]; resp[p++] = ip[1]; resp[p++] = ip[2]; resp[p++] = ip[3];
    backfillRdlen(resp, rdlenPos, p);
  }

  ipaddress_t mcast(MDNS_MULTICAST_ADDR_0, MDNS_MULTICAST_ADDR_1, MDNS_MULTICAST_ADDR_2, MDNS_MULTICAST_ADDR_3);
  m_udp->send(resp, p, mcast, MDNS_PORT);
  pdiutil::safe_delete_array(resp);
}

void MdnsServiceProvider::announce() {
  sendAResponse();
  for (uint8_t i = 0; i < m_service_count; i++) {
    sendServiceBundle(i);
  }
}

void MdnsServiceProvider::onUdpPacket(void *arg) {

  udp_packet_t *pkt = (udp_packet_t *)arg;
  if (nullptr == pkt || pkt->m_len < 12) return;

  const uint8_t *b = pkt->m_data;
  uint16_t len = pkt->m_len;

  // ignore responses (QR bit set)
  if (b[2] & 0x80) return;

  uint16_t qdcount = ((uint16_t)b[4] << 8) | b[5];
  uint16_t off = 12;

  for (uint16_t q = 0; q < qdcount && off < len; q++) {

    // collect the QNAME labels (offset+len into the packet)
    uint16_t lblOff[6];
    uint8_t  lblLen[6];
    uint8_t  n = 0;
    bool     bad = false;
    while (off < len) {
      uint8_t l = b[off++];
      if (l == 0) break;
      if ((l & 0xC0) == 0xC0) { if (off < len) off++; bad = true; break; }
      if (off + l > len || n >= 6) { bad = true; break; }
      lblOff[n] = off; lblLen[n] = l; n++;
      off += l;
    }
    if (off + 4 > len) return;
    uint16_t qtype = ((uint16_t)b[off] << 8) | b[off + 1];
    off += 4; // qtype(2) + qclass(2)

    if (bad || n < 2) continue;
    // every name we answer ends in "local"
    if (!labelEqualsCI(b + lblOff[n - 1], lblLen[n - 1], MDNS_LOCAL_LABEL)) continue;

    // A : <host>.local
    if (n == 2 &&
        labelEqualsCI(b + lblOff[0], lblLen[0], m_hostname.c_str()) &&
        (qtype == MDNS_TYPE_A || qtype == MDNS_TYPE_ANY)) {
      sendAResponse();
      return;
    }

    // service enumeration : _services._dns-sd._udp.local
    if (n == 4 &&
        labelEqualsCI(b + lblOff[0], lblLen[0], "_services") &&
        labelEqualsCI(b + lblOff[1], lblLen[1], "_dns-sd") &&
        labelEqualsCI(b + lblOff[2], lblLen[2], "_udp") &&
        (qtype == MDNS_TYPE_PTR || qtype == MDNS_TYPE_ANY)) {
      sendServiceEnumeration();
      return;
    }

    // service-type browse : <type>._tcp.local  (PTR)
    if (n == 3 && (qtype == MDNS_TYPE_PTR || qtype == MDNS_TYPE_ANY)) {
      for (uint8_t i = 0; i < m_service_count; i++) {
        if (labelEqualsCI(b + lblOff[0], lblLen[0], m_services[i].m_type) &&
            labelEqualsCI(b + lblOff[1], lblLen[1], m_services[i].m_proto)) {
          sendServiceBundle(i);
          return;
        }
      }
    }

    // instance resolve : <host>.<type>._tcp.local  (SRV/TXT)
    if (n == 4 &&
        labelEqualsCI(b + lblOff[0], lblLen[0], m_hostname.c_str()) &&
        (qtype == MDNS_TYPE_SRV || qtype == MDNS_TYPE_TXT || qtype == MDNS_TYPE_ANY)) {
      for (uint8_t i = 0; i < m_service_count; i++) {
        if (labelEqualsCI(b + lblOff[1], lblLen[1], m_services[i].m_type) &&
            labelEqualsCI(b + lblOff[2], lblLen[2], m_services[i].m_proto)) {
          sendServiceBundle(i);
          return;
        }
      }
    }
  }
}

void MdnsServiceProvider::printStatusToTerminal(iTerminalInterface *terminal) {

  if (nullptr == terminal) return;

  terminal->write_ro(RODT_ATTR("  hostname : "));
  terminal->write(m_hostname.c_str());
  terminal->writeln_ro(RODT_ATTR(".local"));

  if (nullptr != m_udp) {
    pdiutil::string ipstr = m_ip;
    terminal->write_ro(RODT_ATTR("  address  : "));
    terminal->writeln(ipstr.c_str());
  }

  if (m_service_count > 0) {
    terminal->write_ro(RODT_ATTR("  services : "));
    for (uint8_t i = 0; i < m_service_count; i++) {
      terminal->write(m_services[i].m_type);
      terminal->write(m_services[i].m_proto);
      terminal->write_ro(RODT_ATTR(":"));
      terminal->write((int32_t)m_services[i].m_port);
      terminal->write_ro(RODT_ATTR(" "));
    }
    terminal->putln();
  }
}

MdnsServiceProvider __mdns_service;

#endif
