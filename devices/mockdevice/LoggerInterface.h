/*********************** Device logging Interface *****************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _MOCKDEVICE_LOGGER_INTERFACE_H_
#define _MOCKDEVICE_LOGGER_INTERFACE_H_

#include "mockdevice.h"
#include <interface/pdi/iLoggerInterface.h>


#ifdef LogI
#undef LogI
#define LogI(v) __i_logger.log_info(RODT_ATTR(v))
#endif

#ifdef LogE
#undef LogE
#define LogE(v) __i_logger.log_error(RODT_ATTR(v))
#endif

#ifdef LogW
#undef LogW
#define LogW(v) __i_logger.log_warning(RODT_ATTR(v))
#endif

#ifdef LogS
#undef LogS
#define LogS(v) __i_logger.log_success(RODT_ATTR(v))
#endif

#ifdef LogFmtI
#undef LogFmtI
#define LogFmtI(f, args...) __i_logger.log_format(RODT_ATTR(f), INFO_LOG, args)
#endif

#ifdef LogFmtE
#undef LogFmtE
#define LogFmtE(f, args...) __i_logger.log_format(RODT_ATTR(f), ERROR_LOG, args)
#endif

#ifdef LogFmtW
#undef LogFmtW
#define LogFmtW(f, args...) __i_logger.log_format(RODT_ATTR(f), WARNING_LOG, args)
#endif

#ifdef LogFmtS
#undef LogFmtS
#define LogFmtS(f, args...) __i_logger.log_format(RODT_ATTR(f), SUCCESS_LOG, args)
#endif

/**
 * LoggerInterface class
 */
class LoggerInterface : public iLoggerInterface
{

public:
  /**
   * iLoggerInterface constructor.
   */
  LoggerInterface(){}

  /**
   * LoggerInterface destructor.
   */
  virtual ~LoggerInterface(){}

  void init() override{}
  void log(logger_type_t log_type, const char *content) override{}
  void log_info(const char *info) override{}
  void log_error(const char *error) override{}
  void log_warning(const char *warning) override{}
  void log_success(const char *success) override{}
  void log_format(const char *format, logger_type_t log_type, ...) override{}
};

#endif
