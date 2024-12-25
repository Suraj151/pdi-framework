/*********************** Device logging Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _ARDUINOUNO_LOGGER_INTERFACE_H_
#define _ARDUINOUNO_LOGGER_INTERFACE_H_

#include "arduinouno.h"
#include <interface/pdi/iLoggerInterface.h>


#if defined(LOGBEGIN) && ( defined(ENABLE_LOG_ALL) || defined(ENABLE_LOG_INFO) || defined(ENABLE_LOG_ERROR) || defined(ENABLE_LOG_WARNING) || defined(ENABLE_LOG_SUCCESS) )
#undef LOGBEGIN
#define LOGBEGIN __i_logger.init()
#endif

#if defined(LogI) && ( defined(ENABLE_LOG_INFO) || defined(ENABLE_LOG_ALL) )
#undef LogI
#define LogI(v) __i_logger.log_info(RODT_ATTR(v))
#endif

#if defined(LogE) && ( defined(ENABLE_LOG_ERROR) || defined(ENABLE_LOG_ALL) )
#undef LogE
#define LogE(v) __i_logger.log_error(RODT_ATTR(v))
#endif

#if defined(LogW) && ( defined(ENABLE_LOG_WARNING) || defined(ENABLE_LOG_ALL) )
#undef LogW
#define LogW(v) __i_logger.log_warning(RODT_ATTR(v))
#endif

#if defined(LogS) && ( defined(ENABLE_LOG_SUCCESS) || defined(ENABLE_LOG_ALL) )
#undef LogS
#define LogS(v) __i_logger.log_success(RODT_ATTR(v))
#endif

#if defined(LogFmtI) && ( defined(ENABLE_LOG_INFO) || defined(ENABLE_LOG_ALL) )
#undef LogFmtI
#define LogFmtI(f, args...) __i_logger.log_format(RODT_ATTR(f), INFO_LOG, args)
#endif

#if defined(LogFmtE) && ( defined(ENABLE_LOG_ERROR) || defined(ENABLE_LOG_ALL) )
#undef LogFmtE
#define LogFmtE(f, args...) __i_logger.log_format(RODT_ATTR(f), ERROR_LOG, args)
#endif

#if defined(LogFmtW) && ( defined(ENABLE_LOG_WARNING) || defined(ENABLE_LOG_ALL) )
#undef LogFmtW
#define LogFmtW(f, args...) __i_logger.log_format(RODT_ATTR(f), WARNING_LOG, args)
#endif

#if defined(LogFmtS) && ( defined(ENABLE_LOG_SUCCESS) || defined(ENABLE_LOG_ALL) )
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
  LoggerInterface();

  /**
   * LoggerInterface destructor.
   */
  virtual ~LoggerInterface();

  void init() override;
  void log(logger_type_t log_type, const char *content) override;
  void log_info(const char *info) override;
  void log_error(const char *error) override;
  void log_warning(const char *warning) override;
  void log_success(const char *success) override;
  void log_format(const char *format, logger_type_t log_type, ...) override;
};

#endif
