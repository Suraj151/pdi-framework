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
#endif

LogManager::LogManager() : m_io(nullptr)
#ifdef ENABLE_SYSLOG_SERVICE
  , m_sink(nullptr)
#endif
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

static inline bool isConsoleLogEnabled(logger_type_t log_type)
{
#if defined(ENABLE_CONSOLE_LOG_ALL)
  (void)log_type;
  return true;
#else
#if defined(ENABLE_CONSOLE_LOG_INFO)
  if (INFO_LOG == log_type) return true;
#endif
#if defined(ENABLE_CONSOLE_LOG_ERROR)
  if (ERROR_LOG == log_type) return true;
#endif
#if defined(ENABLE_CONSOLE_LOG_WARNING)
  if (WARNING_LOG == log_type) return true;
#endif
#if defined(ENABLE_CONSOLE_LOG_SUCCESS)
  if (SUCCESS_LOG == log_type) return true;
#endif
  (void)log_type;
  return false;
#endif
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

  if (nullptr != m_io && isConsoleLogEnabled(log_type))
  {
    m_io->write(line);   // console (via the held io terminal)
  }
  if (nullptr != m_sink)
  {
    m_sink(log_type, line, len);   // file + remote (via the syslog service)
  }
}

#endif

LogManager __log_manager;
