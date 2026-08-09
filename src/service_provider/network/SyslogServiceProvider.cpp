/******************************* Syslog service ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 26th July 2026
******************************************************************************/

#include "SyslogServiceProvider.h"

#ifdef ENABLE_SYSLOG_SERVICE

#include <interface/pdi/impl/modules/storage/VfsDispatcher.h>
#include <interface/pdi/middlewares/iNtpInterface.h>
#include <utility/DataTypeConversions.h>

#ifdef ENABLE_SYSLOG_FORWARD
#include <interface/pdi/middlewares/iUdpInterface.h>
#include <service_provider/network/NameResolver.h>
#include <utility/EventUtil.h>
#endif

// guards against re-entry (an FS/socket-path log must not recurse into the sink)
static bool s_syslog_busy = false;

SyslogServiceProvider::SyslogServiceProvider() : ServiceProvider(SERVICE_SYSLOG, "SYSLOG")
#ifdef ENABLE_SYSLOG_FORWARD
  , m_udp(nullptr)
  , m_port(SYSLOG_REMOTE_PORT)
  , m_resolved(false)
#endif
{
}

SyslogServiceProvider::~SyslogServiceProvider() {
  stopService();
}

bool SyslogServiceProvider::initService(void *arg) {

  // become the LogManager syslog sink
  __log_manager.setSyslogSink(&SyslogServiceProvider::sink);

#ifdef ENABLE_SYSLOG_FORWARD
  m_host = SYSLOG_REMOTE_HOST;
  m_port = SYSLOG_REMOTE_PORT;

  // (re)resolve the collector whenever the station acquires an IP
  __utl_event.add_event_listener(EVENT_WIFI_STA_GOT_IP, [](void *a) {
    __syslog_service.resolveCollector();
  });

  if (__i_wifi.localIP().isSet()) {
    resolveCollector();
  }
#endif

  return ServiceProvider::initService(arg);
}

bool SyslogServiceProvider::stopService() {

  __log_manager.setSyslogSink(nullptr);

#ifdef ENABLE_SYSLOG_FORWARD
  if (nullptr != m_udp) {
    m_udp->close();
    pdiutil::safe_delete(m_udp);
    m_udp = nullptr;
  }
  m_resolved = false;
#endif
  return ServiceProvider::stopService();
}

void SyslogServiceProvider::sink(logger_type_t log_type, const char *line, uint16_t len) {
  __syslog_service.handleLine(log_type, line, len);
}

void SyslogServiceProvider::handleLine(logger_type_t log_type, const char *line, uint16_t len) {

  if (s_syslog_busy || nullptr == line || 0 == len) return;
  s_syslog_busy = true;

  writeToFile(log_type, line, len);
#ifdef ENABLE_SYSLOG_FORWARD
  forward(log_type, line, len);
#endif

  s_syslog_busy = false;
}

const char *SyslogServiceProvider::fileForType(logger_type_t type) {
  switch (type) {
    case ERROR_LOG:   return SYSLOG_FILE_ERROR;
    case WARNING_LOG: return SYSLOG_FILE_WARNING;
    case SUCCESS_LOG: return SYSLOG_FILE_SUCCESS;
    case INFO_LOG:
    default:          return SYSLOG_FILE_INFO;
  }
}

void SyslogServiceProvider::writeToFile(logger_type_t type, const char *line, uint16_t len) {

  // ensure the log directory once (storage is mounted before this service inits)
  if (!__i_fs.isDirExist(SYSLOG_DIR)) {
    __i_fs.createDirectory(SYSLOG_DIR);
  }

  const char *path = fileForType(type);
  // append while under the threshold; once crossed, start the file over
  int64_t sz = __i_fs.getFileSize(path);
  bool append = (sz >= 0 && sz < SYSLOG_FILE_MAX_SIZE);

  // one timestamp for this entry (NTP-backed; dashes when the clock is unsynced)
  char ts[24];
  uint32_t epoch = __i_ntp.is_valid_ntptime() ? (uint32_t)__i_ntp.get_ntp_time() : 0;
  EpochToDateTimeString(epoch, ts, sizeof(ts), SYSLOG_FILE_TS_FMT);
  uint16_t tslen = (uint16_t)strlen(ts);

  // stamp the start of every line: prefix the entry, re-prefix after each
  // embedded newline, and make sure it ends on its own line
  uint16_t nl = 0;
  for (uint16_t i = 0; i < len; i++) if ('\n' == line[i]) nl++;
  uint32_t outcap = (uint32_t)len + (uint32_t)(nl + 1) * tslen + 2;

  char *out = pdiutil::safe_new_array<char>(outcap);
  if (nullptr == out) {                        // low heap — write the raw line
    __i_fs.writeFile(path, line, len, append);
    return;
  }

  uint32_t o = 0;
  bool at_line_start = true;
  for (uint16_t i = 0; i < len; i++) {
    char c = line[i];
    if (at_line_start && '\n' != c) {
      memcpy(out + o, ts, tslen);
      o += tslen;
      at_line_start = false;
    }
    out[o++] = c;
    if ('\n' == c) at_line_start = true;
  }
  if (0 == o || '\n' != out[o - 1]) out[o++] = '\n';

  __i_fs.writeFile(path, out, (uint32_t)o, append);
  pdiutil::safe_delete_array(out);
}

#ifdef ENABLE_SYSLOG_FORWARD

bool SyslogServiceProvider::resolveCollector() {

  m_resolved = false;
  if (0 == m_host.length()) return false;

  if (NameResolver::parseIpLiteral(m_host.c_str(), m_collector_ip) ||
      NameResolver::resolve(m_host.c_str(), m_collector_ip)) {
    m_resolved = true;
  }
  return m_resolved;
}

bool SyslogServiceProvider::ensureSocket() {

  if (nullptr != m_udp) return true;

  m_udp = __i_instance.getNewUdpInstance();
  if (nullptr == m_udp) return false; // port without a UDP impl yet

  if (!m_udp->begin(0)) {             // ephemeral local port, send only
    pdiutil::safe_delete(m_udp);
    m_udp = nullptr;
    return false;
  }
  return true;
}

uint8_t SyslogServiceProvider::priorityFor(logger_type_t log_type) {

  uint8_t severity;
  switch (log_type) {
    case ERROR_LOG:   severity = 3; break; // err
    case WARNING_LOG: severity = 4; break; // warning
    case SUCCESS_LOG: severity = 5; break; // notice
    case INFO_LOG:
    default:          severity = 6; break; // info
  }
  return (uint8_t)(SYSLOG_FORWARD_FACILITY * 8 + severity);
}

void SyslogServiceProvider::forward(logger_type_t log_type, const char *line, uint16_t len) {

  if (!m_resolved || !ensureSocket()) return;

  char dgram[SYSLOG_FORWARD_DGRAM_MAX];
  pdiutil::string host = __i_wifi.localIP();
  int pri = priorityFor(log_type);

  // RFC 3164 header: <PRI>TIMESTAMP HOSTNAME TAG:  (timestamp only when NTP is valid)
  int hlen;
  if (__i_ntp.is_valid_ntptime()) {
    char ts[16];
    EpochToDateTimeString((uint32_t)__i_ntp.get_ntp_time(), ts, sizeof(ts), "%b %d %H:%M:%S");
    hlen = __snprintf(dgram, SYSLOG_FORWARD_DGRAM_MAX, "<%d>%s %s %s: ", pri, ts, host.c_str(), SYSLOG_FORWARD_TAG);
  } else {
    hlen = __snprintf(dgram, SYSLOG_FORWARD_DGRAM_MAX, "<%d>%s %s: ", pri, host.c_str(), SYSLOG_FORWARD_TAG);
  }

  if (hlen > 0) {
    if (hlen >= SYSLOG_FORWARD_DGRAM_MAX) hlen = SYSLOG_FORWARD_DGRAM_MAX - 1;

    // append the message, minus any trailing newline, up to the datagram cap
    uint16_t mlen = len;
    while (mlen > 0 && ('\n' == line[mlen - 1] || '\r' == line[mlen - 1])) mlen--;
    uint16_t cap = (uint16_t)(SYSLOG_FORWARD_DGRAM_MAX - hlen);
    if (mlen > cap) mlen = cap;
    memcpy(dgram + hlen, line, mlen);

    m_udp->send((const uint8_t *)dgram, (uint16_t)(hlen + mlen), m_collector_ip, m_port);
  }
}

#endif

void SyslogServiceProvider::printStatusToTerminal(iTerminalInterface *terminal) {

  if (nullptr == terminal) return;

  terminal->write_ro(RODT_ATTR("  logdir    : "));
  terminal->writeln_ro(RODT_ATTR(SYSLOG_DIR));

#ifdef ENABLE_SYSLOG_FORWARD
  terminal->write_ro(RODT_ATTR("  collector : "));
  if (m_host.length() > 0) {
    terminal->write(m_host.c_str());
    terminal->write_ro(RODT_ATTR(":"));
    terminal->writeln((int32_t)m_port);
  } else {
    terminal->writeln_ro(RODT_ATTR("(none)"));
  }

  terminal->write_ro(RODT_ATTR("  resolved  : "));
  if (m_resolved) {
    pdiutil::string ipstr = m_collector_ip;
    terminal->writeln(ipstr.c_str());
  } else {
    terminal->writeln_ro(RODT_ATTR("no"));
  }

  terminal->write_ro(RODT_ATTR("  socket    : "));
  if (nullptr != m_udp) {
    terminal->writeln_ro(RODT_ATTR("open"));
  } else {
    terminal->writeln_ro(RODT_ATTR("closed"));
  }
#endif
}

SyslogServiceProvider __syslog_service;

#endif
