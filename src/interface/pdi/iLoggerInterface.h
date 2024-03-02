/*********************** Device logging Interface *****************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_LOGGER_INTERFACE_H_
#define _I_LOGGER_INTERFACE_H_

#include <interface/interface_includes.h>


// forward declaration of derived class for this interface
class LoggerInterface;

/**
 * iLoggerInterface class
 */
class iLoggerInterface
{

public:
  /**
   * iLoggerInterface constructor.
   */
  iLoggerInterface() {}
  /**
   * iLoggerInterface destructor.
   */
  virtual ~iLoggerInterface() {}

  // init the required steps to perform before log starts
  virtual void init() = 0;
  virtual void log(logger_type_t log_type, const char *content) = 0;
  virtual void log_info(const char *info) = 0;
  virtual void log_error(const char *error) = 0;
  virtual void log_warning(const char *warning) = 0;
  virtual void log_success(const char *success) = 0;
  virtual void log_format(const char *format, logger_type_t log_type, ...) = 0;
};

// derived class must define this
extern LoggerInterface __i_logger;


// redefine these in derived interface
#define LOGBEGIN __i_logger.init()

#define LogI(v) // info log
#define LogE(v) // error log
#define LogW(v) // warning log
#define LogS(v) // success log

#define LogFmtI(f, args...)
#define LogFmtE(f, args...)
#define LogFmtW(f, args...)
#define LogFmtS(f, args...)

#endif
