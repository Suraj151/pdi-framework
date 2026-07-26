/******************************* Syslog service ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 26th July 2026
******************************************************************************/

#ifndef _SYSLOG_SERVICE_PROVIDER_H_
#define _SYSLOG_SERVICE_PROVIDER_H_

#include <service_provider/ServiceProvider.h>

#ifdef ENABLE_SYSLOG_SERVICE

/**
 * SyslogServiceProvider class
 *
 * The sink behind every SysLog* line. LogManager assembles a line, echoes it to
 * the console, then hands it here; this service persists it to
 * /var/log/syslog.<type> (append with truncate-at-size rotation). When
 * ENABLE_SYSLOG_FORWARD is set it additionally ships the same line to a remote
 * collector as an RFC 3164 datagram over UDP (built on iUdpInterface, no vendor
 * library); the collector is (re)resolved via NameResolver on station-got-IP.
 */
class SyslogServiceProvider : public ServiceProvider
{

public:
  SyslogServiceProvider();
  ~SyslogServiceProvider();

  bool initService(void *arg = nullptr) override;
  bool stopService() override;
  void printStatusToTerminal(iTerminalInterface *terminal) override;

  /* persist one assembled syslog line to file (and forward it when enabled) */
  void handleLine(logger_type_t log_type, const char *line, uint16_t len);

protected:
  /* LogManager sink adapter — delegates to the singleton */
  static void sink(logger_type_t log_type, const char *line, uint16_t len);

  void writeToFile(logger_type_t log_type, const char *line, uint16_t len);
  const char *fileForType(logger_type_t log_type);

#ifdef ENABLE_SYSLOG_FORWARD
  bool ensureSocket(void);
  bool resolveCollector(void);
  uint8_t priorityFor(logger_type_t log_type);
  void forward(logger_type_t log_type, const char *line, uint16_t len);

  iUdpInterface  *m_udp;
  pdiutil::string m_host;
  uint16_t        m_port;
  ipaddress_t     m_collector_ip;
  bool            m_resolved;
#endif
};

extern SyslogServiceProvider __syslog_service;

#endif

#endif
