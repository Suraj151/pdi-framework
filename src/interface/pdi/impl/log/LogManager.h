/***************************** Log Manager ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 25th July 2026
******************************************************************************/

#ifndef _LOG_MANAGER_H_
#define _LOG_MANAGER_H_

#include <config/Config.h>
#include <interface/pdi/iLoggerInterface.h>

/**
 * LogManager (interface-layer, common to all ports)
 *
 * The single logger for the whole stack. `log` streams a formatted line to the
 * held io (serial terminal) character by character — no line buffer, so it fits
 * even the smallest RAM. `syslog` formats the line, echoes it to the same io,
 * then hands it to a registered sink (the syslog service) which owns file
 * persistence and any remote forwarding; it is compiled only when the
 * storage-backed syslog feature is enabled.
 */
class LogManager : public iLoggerInterface
{

public:
  LogManager();
  ~LogManager();

  /* store the io terminal (and open it) — call once from the stack init */
  void init(iIOInterface *io = nullptr) override;

  /* format `format` (RO/PROGMEM) + args and stream it to the io */
  void log(logger_type_t log_type, const char *format, ...) override;

#ifdef ENABLE_SYSLOG_SERVICE

  /* the syslog line sink — a service registers here to persist the assembled
     line to file (and optionally forward it); nullptr = console-only syslog */
  typedef void (*SyslogSinkFn)(logger_type_t log_type, const char *line, uint16_t len);
  void setSyslogSink(SyslogSinkFn sink) { m_sink = sink; }

  /* like log, but also hands the assembled line to the sink */
  void syslog(logger_type_t log_type, const char *format, ...);

#endif

private:
  iIOInterface *m_io;
#ifdef ENABLE_SYSLOG_SERVICE
  SyslogSinkFn m_sink;
#endif
};

extern LogManager __log_manager;

#endif
