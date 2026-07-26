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
 * even the smallest RAM. `syslog` additionally persists the line to
 * /var/log/syslog.<type> and is compiled only when the storage-backed syslog
 * feature is enabled.
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

  /* like log, but also append the assembled line to the type's syslog file */
  void syslog(logger_type_t log_type, const char *format, ...);

protected:
  void writeLine(logger_type_t log_type, const char *line, uint16_t len);
  const char *fileForType(logger_type_t log_type);

#endif

private:
  iIOInterface *m_io;
};

extern LogManager __log_manager;

#endif
