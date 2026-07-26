/***************************** Log Manager ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 25th July 2026
******************************************************************************/

#include "LogManager.h"
#include <stdarg.h>

#ifdef ENABLE_SYSLOG_SERVICE
#include <utility/StringOperations.h>
#include <interface/pdi/impl/modules/storage/VfsDispatcher.h>
// guards against re-entry (e.g. an FS-internal log while we are mid-write)
static bool s_syslog_busy = false;
#endif

LogManager::LogManager() : m_io(nullptr)
{
}

LogManager::~LogManager()
{
  m_io = nullptr;
}

void LogManager::init(iIOInterface *io)
{
  m_io = io;
  if (nullptr != m_io && !m_io->isopen())
  {
    m_io->open(0, 115200);
  }
}

void LogManager::log(logger_type_t log_type, const char *format, ...)
{
  if (nullptr == m_io)
  {
    return;
  }

  // RO/PROGMEM format -> dynamic RAM (auto-freed) -> owned string
  pdiutil::string fmtStr = CHARPTR_WRAP_RO(format);
  const char *fmt = fmtStr.c_str();
  size_t fmtLen = fmtStr.length();

  va_list args;
  va_start(args, format);

  for (size_t indx = 0; indx < fmtLen; indx++)
  {
    if (fmt[indx] == '%' && indx < (fmtLen - 1))
    {
      ++indx;

      if (fmt[indx] == 'd')
      {
        int i = va_arg(args, int);
        m_io->write((int32_t)i);
      }
      else if (fmt[indx] == 'u')
      {
        unsigned int u = va_arg(args, unsigned int);
        m_io->write((uint32_t)u);
      }
      else if (fmt[indx] == 'x')
      {
        int x = va_arg(args, int);
        m_io->write((uint32_t)x, true);
      }
      else if (fmt[indx] == 'c')
      {
        // A 'char' variable will be promoted to 'int'
        // A character literal in C is already 'int' by itself
        int c = va_arg(args, int);
        m_io->write((char)c);
      }
      else if (fmt[indx] == 'f')
      {
        double d = va_arg(args, double);
        m_io->write(d);
      }
      else if (fmt[indx] == 's')
      {
        char *s = va_arg(args, char*);
        if (nullptr != s)
        {
          m_io->write(s);
        }
      }
      else
      {
        --indx;
        m_io->write(fmt[indx]);
      }
    }
    else
    {
      m_io->write(fmt[indx]);
    }
  }

  va_end(args);
}

#ifdef ENABLE_SYSLOG_SERVICE

const char *LogManager::fileForType(logger_type_t type)
{
  switch (type)
  {
    case ERROR_LOG:   return SYSLOG_FILE_ERROR;
    case WARNING_LOG: return SYSLOG_FILE_WARNING;
    case SUCCESS_LOG: return SYSLOG_FILE_SUCCESS;
    case INFO_LOG:
    default:          return SYSLOG_FILE_INFO;
  }
}

void LogManager::writeLine(logger_type_t type, const char *line, uint16_t len)
{
  if (s_syslog_busy || nullptr == line || 0 == len) return;
  s_syslog_busy = true;

  // ensure the log directory once (storage is mounted after log init)
  if (!__i_fs.isDirExist(SYSLOG_DIR))
  {
    __i_fs.createDirectory(SYSLOG_DIR);
  }

  const char *path = fileForType(type);
  // append while under the threshold; once crossed, start the file over
  int64_t sz = __i_fs.getFileSize(path);
  bool append = (sz >= 0 && sz < SYSLOG_FILE_MAX_SIZE);
  __i_fs.writeFile(path, line, len, append);

  s_syslog_busy = false;
}

void LogManager::syslog(logger_type_t log_type, const char *format, ...)
{
  // RO/PROGMEM format -> dynamic RAM (auto-freed) -> owned string
  pdiutil::string fmtStr = CHARPTR_WRAP_RO(format);

  char line[SYSLOG_LINE_MAX];
  va_list args;
  va_start(args, format);
  int n = __vsnprintf(line, SYSLOG_LINE_MAX, fmtStr.c_str(), args);
  va_end(args);
  if (n <= 0) return;

  uint16_t len = (n >= SYSLOG_LINE_MAX) ? (uint16_t)(SYSLOG_LINE_MAX - 1) : (uint16_t)n;

  if (nullptr != m_io)
  {
    m_io->write(line);   // console (via the held io terminal)
  }
  writeLine(log_type, line, len);   // file
}

#endif

LogManager __log_manager;
